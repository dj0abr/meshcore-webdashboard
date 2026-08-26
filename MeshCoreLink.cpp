#include "MeshCoreLink.h"

#include <chrono>
#include <iomanip>
#include <iostream>

static bool startsWith(const std::string &s, const std::string &prefix)
{
    return s.rfind(prefix, 0) == 0;
}

static bool containsCode(const std::vector<uint8_t>& codes, uint8_t code)
{
    for (uint8_t c : codes)
    {
        if (c == code)
        {
            return true;
        }
    }

    return false;
}

MeshCoreLink::MeshCoreLink()
{
    m_running = false;
    m_waiting = false;
}

MeshCoreLink::~MeshCoreLink()
{
    stop();
}

bool MeshCoreLink::start(const std::string &device)
{
    stop();

    if (startsWith(device, "tcp://"))
    {
        std::string target = device.substr(6);

        std::string host = target;
        uint16_t port = 0;

        size_t colon = target.rfind(':');

        if (colon != std::string::npos)
        {
            host = target.substr(0, colon);
            port = static_cast<uint16_t>(std::stoi(target.substr(colon + 1)));
        }
        else
        {
            return false;
        }

        auto tcp = std::make_unique<TcpPort>();

        if (!tcp->open(host, port))
        {
            return false;
        }

        m_stream = std::move(tcp);
    }
    else
    {
        auto serial = std::make_unique<SerialPort>();

        if (!serial->open(device))
        {
            return false;
        }

        m_stream = std::move(serial);
    }

    m_framer.emplace(*m_stream);

    m_running = true;
    m_rxThread = std::thread(&MeshCoreLink::rxLoop, this);

    return true;
}

void MeshCoreLink::stop()
{
    if (m_running)
    {
        m_running = false;

        m_reqCv.notify_all();

        if (m_rxThread.joinable())
        {
            m_rxThread.join();
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_reqMutex);
        m_waiting = false;
        m_lastResp.clear();
        m_wantedCodes.clear();
    }

    m_framer.reset();

    if (m_stream)
    {
        m_stream->close();
        m_stream.reset();
    }
}

bool MeshCoreLink::isRunning() const
{
    return m_running.load() && m_stream && m_stream->isOpen();
}

void MeshCoreLink::setPushCallback(PushCallback cb)
{
    std::lock_guard<std::mutex> lock(m_cbMutex);
    m_pushCb = std::move(cb);
}

std::optional<std::vector<uint8_t>> MeshCoreLink::requestResponse(
    const std::vector<uint8_t> &cmdPayload,
    uint8_t wantedCode,
    int timeoutMs)
{
    return requestResponseAny(cmdPayload, std::vector<uint8_t> { wantedCode }, timeoutMs);
}

std::optional<std::vector<uint8_t>> MeshCoreLink::requestResponseAny(
    const std::vector<uint8_t> &cmdPayload,
    const std::vector<uint8_t> &wantedCodes,
    int timeoutMs)
{
    return requestResponseMatching(cmdPayload, wantedCodes, nullptr, timeoutMs);
}

std::optional<std::vector<uint8_t>> MeshCoreLink::requestResponseMatching(
    const std::vector<uint8_t> &cmdPayload,
    const std::vector<uint8_t> &wantedCodes,
    const std::function<bool(const std::vector<uint8_t>&)> &matcher,
    int timeoutMs)
{
    if (!isRunning() || !m_framer.has_value())
    {
        return std::nullopt;
    }

    if (wantedCodes.empty())
    {
        return std::nullopt;
    }

    std::unique_lock<std::mutex> lock(m_reqMutex);

    if (m_waiting)
    {
        return std::nullopt;
    }

    m_waiting = true;
    m_wantedCodes = wantedCodes;
    m_responseMatcher = matcher;
    m_lastResp.clear();

    lock.unlock();

    if (!m_framer->sendPayload(cmdPayload))
    {
        std::lock_guard<std::mutex> g(m_reqMutex);
        m_waiting = false;
        m_wantedCodes.clear();
        m_responseMatcher = nullptr;
        return std::nullopt;
    }

    lock.lock();

    bool ok = m_reqCv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&]()
    {
        if (m_lastResp.empty())
        {
            return false;
        }

        return containsCode(m_wantedCodes, m_lastResp[0]);
    });

    m_waiting = false;
    m_wantedCodes.clear();
    m_responseMatcher = nullptr;

    if (!ok)
    {
        m_lastResp.clear();
        return std::nullopt;
    }

    auto out = m_lastResp;
    m_lastResp.clear();
    return out;
}

std::optional<std::vector<uint8_t>> MeshCoreLink::waitResponseMatching(
    const std::vector<uint8_t> &wantedCodes,
    const std::function<bool(const std::vector<uint8_t>&)> &matcher,
    int timeoutMs)
{
    if (!isRunning())
    {
        std::cerr << "[link-wait] rejected: link not running\n";
        return std::nullopt;
    }

    if (wantedCodes.empty())
    {
        std::cerr << "[link-wait] rejected: no wanted codes\n";
        return std::nullopt;
    }

    std::unique_lock<std::mutex> lock(m_reqMutex);

    if (m_waiting)
    {
        std::cerr << "[link-wait] rejected: another response waiter is already active\n";
        return std::nullopt;
    }

    m_waiting = true;
    m_wantedCodes = wantedCodes;
    m_responseMatcher = matcher;
    m_lastResp.clear();

    bool ok = m_reqCv.wait_for(lock, std::chrono::milliseconds(timeoutMs), [&]()
    {
        if (m_lastResp.empty())
        {
            return false;
        }

        return containsCode(m_wantedCodes, m_lastResp[0]);
    });

    m_waiting = false;
    m_wantedCodes.clear();
    m_responseMatcher = nullptr;

    if (!ok)
    {
        m_lastResp.clear();
        return std::nullopt;
    }

    auto out = m_lastResp;
    m_lastResp.clear();
    return out;
}

std::optional<std::vector<uint8_t>> MeshCoreLink::readOneFrame(int timeoutMs)
{
    if (!isRunning() || !m_framer.has_value())
    {
        return std::nullopt;
    }

    return m_framer->readPayload(timeoutMs);
}

void MeshCoreLink::rxLoop()
{
    while (m_running)
    {
        if (!m_framer.has_value())
        {
            continue;
        }

        auto frame = m_framer->readPayload(500);
        if (!frame.has_value() || frame->empty())
        {
            continue;
        }

        uint8_t code = (*frame)[0];

        // Low-level diagnostics for the repeater login path. This runs before
        // request matching and before any user callback, so it also tells us
        // whether the RX thread is stuck while dispatching an earlier frame.
        if (code == 0x88)
        {
            std::cout << "[link-rx] code=0x88 len=" << frame->size() << "\n";
        }
        else if (code == 0x81 || code == 0x85 || code == 0x86)
        {
            std::cout
                << "[link-rx] code=0x"
                << std::hex
                << std::setw(2)
                << std::setfill('0')
                << static_cast<unsigned>(code)
                << std::dec
                << " len="
                << frame->size()
                << " raw=";

            for (uint8_t b : *frame)
            {
                std::cout
                    << std::hex
                    << std::setw(2)
                    << std::setfill('0')
                    << static_cast<unsigned>(b);
            }

            std::cout << std::dec << "\n";
        }

        // Fulfill request/response waiters
        {
            std::lock_guard<std::mutex> lock(m_reqMutex);

            if (m_waiting && containsCode(m_wantedCodes, code))
            {
                bool matches = true;

                if (m_responseMatcher)
                {
                    matches = m_responseMatcher(*frame);
                }

                if (matches)
                {
                    m_lastResp = *frame;
                    m_reqCv.notify_all();
                    continue;
                }
            }

            // Sonderfall listPeers: wanted=RESP_CODE_CONTACT, aber END soll auch erfüllen
            // -> MeshCoreClient kann das weiterhin selbst machen, ODER wir lassen das hier drin:
            // Wenn du es hier drin lassen willst, gib MeshCoreLink den END-Code als "auch ok".
        }

        // User push callback
        PushCallback cb;
        {
            std::lock_guard<std::mutex> lock(m_cbMutex);
            cb = m_pushCb;
        }

        if (cb)
        {
            cb(code, *frame);
        }
    }
}