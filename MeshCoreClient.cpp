#include "MeshCoreClient.h"
#include "MeshDB.h"
#include "MessageCorrelation.h"

#include <chrono>
#include <cmath>
#include <cstring>
#include <ctime>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <iomanip>


extern void debugPrintContactFrame(const std::vector<uint8_t>& frame);

#include <iomanip>
#include <iostream>
#include <sstream>
#include <cmath>

namespace
{
    std::string BytesToHex(const std::vector<uint8_t>& data)
    {
        std::ostringstream oss;

        for (uint8_t b : data)
        {
            oss << std::hex
                << std::setw(2)
                << std::setfill('0')
                << static_cast<unsigned>(b);
        }

        return oss.str();
    }

    std::string Prefix6ToHex(const std::array<uint8_t, 6>& pfx)
    {
        std::ostringstream oss;

        for (uint8_t b : pfx)
        {
            oss << std::hex
                << std::setw(2)
                << std::setfill('0')
                << static_cast<unsigned>(b);
        }

        return oss.str();
    }

    const char* RespCodeToString(uint8_t code)
    {
        switch (code)
        {
            case MeshCoreProto::RESP_CODE_CONTACT_MSG_RECV_V3:
                return "RESP_CODE_CONTACT_MSG_RECV_V3";

            case MeshCoreProto::RESP_CODE_CONTACT_MSG_RECV:
                return "RESP_CODE_CONTACT_MSG_RECV";

            case MeshCoreProto::RESP_CODE_CHANNEL_MSG_RECV_V3:
                return "RESP_CODE_CHANNEL_MSG_RECV_V3";

            case MeshCoreProto::RESP_CODE_CHANNEL_MSG_RECV:
                return "RESP_CODE_CHANNEL_MSG_RECV";

            case MeshCoreProto::RESP_CODE_NO_MORE_MESSAGES:
                return "RESP_CODE_NO_MORE_MESSAGES";

            default:
                return "UNKNOWN";
        }
    }

    void DumpFrameBytes(const std::vector<uint8_t>& frame)
    {
        std::cout << "  bytes[" << frame.size() << "] :";

        for (size_t i = 0; i < frame.size(); ++i)
        {
            if ((i % 16) == 0)
            {
                std::cout << "\n    ";
            }

            std::cout << std::hex
                << std::setw(2)
                << std::setfill('0')
                << static_cast<unsigned>(frame[i])
                << " ";
        }

        std::cout << std::dec << "\n";
    }

    void DumpDecodedMessage(const MeshCoreClient::RxMessage& m)
    {
        std::cout << "  decoded:\n";
        std::cout << "    isChannel       : " << (m.isChannel ? 1 : 0) << "\n";
        std::cout << "    channelIdx      : " << static_cast<unsigned>(m.channelIdx) << "\n";
        std::cout << "    txtType         : " << static_cast<unsigned>(m.txtType) << "\n";
        std::cout << "    pathLen         : " << static_cast<unsigned>(m.pathLen) << "\n";

        if (std::isnan(m.snrDb))
        {
            std::cout << "    snrDb           : NaN\n";
        }
        else
        {
            std::cout << "    snrDb           : " << m.snrDb << "\n";
        }

        std::cout << "    senderTimestamp : " << m.senderTimestamp << "\n";
        std::cout << "    senderPrefix6   : " << Prefix6ToHex(m.senderPrefix6) << "\n";
        std::cout << "    textLen         : " << m.text.size() << "\n";
        std::cout << "    text            : [" << m.text << "]\n";
    }
}

static bool ShouldReplaceDiscoverResult(
    const MeshCoreClient::DiscoverResult& oldValue,
    const MeshCoreClient::DiscoverResult& newValue)
{
    if (oldValue.sourceCode == 0x8E && newValue.sourceCode == 0x88)
    {
        return true;
    }

    if (oldValue.sourceCode == 0x88 && newValue.sourceCode == 0x8E)
    {
        return false;
    }

    if (newValue.rssiDbm > oldValue.rssiDbm)
    {
        return true;
    }

    return false;
}
static std::string bytesToHexVec(const std::vector<uint8_t>& data, const char* sep = " ")
{
    std::ostringstream oss;

    for (size_t i = 0; i < data.size(); ++i)
    {
        if (i != 0)
        {
            oss << sep;
        }

        oss
            << std::uppercase
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<unsigned>(data[i]);
    }

    return oss.str();
}

template <size_t N>
static std::string bytesToHexArr(const std::array<uint8_t, N>& data, const char* sep = " ")
{
    std::ostringstream oss;

    for (size_t i = 0; i < N; ++i)
    {
        if (i != 0)
        {
            oss << sep;
        }

        oss
            << std::uppercase
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<unsigned>(data[i]);
    }

    return oss.str();
}

static std::string nodeId8ToHex(const std::array<uint8_t, 8>& value)
{
    return bytesToHexArr(value, "");
}

static std::string nodeIdKey(const std::array<uint8_t, 8>& value)
{
    return nodeId8ToHex(value);
}

// Protocol codes + decoders live in MeshCoreProto.* (included via MeshCoreClient.h)

MeshCoreClient::MeshCoreClient()
{
    m_running = false;
    m_msgSyncPending = false;
    m_enableRxLog = false;

    m_captureContacts = false;
}

MeshCoreClient::~MeshCoreClient()
{
    disconnect();
}

bool MeshCoreClient::connect(const std::string &port)
{
    disconnect();

    if (!m_link.start(port))
    {
        return false;
    }

    {
        std::lock_guard<std::mutex> lock(m_authMutex);
        m_authenticatedPeers.clear();
    }

    m_running = true;

    // Link callback: internal handling first, then user callback
    m_link.setPushCallback([this](uint8_t code, const std::vector<uint8_t> &payload)
    {
        onLinkFrame(code, payload);
    });

    m_pushDispatchThread = std::thread(&MeshCoreClient::pushDispatchLoop, this);
    m_taskThread = std::thread(&MeshCoreClient::taskLoop, this);

    if (!doHandshake())
    {
        disconnect();
        return false;
    }

    return true;
}

void MeshCoreClient::disconnect()
{
    if (m_running)
    {
        m_running = false;

        {
            std::lock_guard<std::mutex> lock(m_taskMutex);
            m_msgSyncPending = true;
        }
        m_taskCv.notify_all();
        m_runCv.notify_all();
        m_pushDispatchCv.notify_all();

        {
            std::lock_guard<std::mutex> lock(m_captureMutex);
            m_captureContacts = false;
            m_captureQueue.clear();
        }
        m_captureCv.notify_all();

        {
            std::lock_guard<std::mutex> lock(m_loginMutex);
            m_loginCapture.active = false;
            m_loginCapture.ready = false;
            m_loginCapture.frame.clear();
        }
        m_loginCv.notify_all();

        {
            std::lock_guard<std::mutex> lock(m_binaryMutex);
            m_binaryCapture.active = false;
            m_binaryCapture.frames.clear();
        }
        m_binaryCv.notify_all();

        if (m_taskThread.joinable())
        {
            m_taskThread.join();
        }

        if (m_pushDispatchThread.joinable())
        {
            m_pushDispatchThread.join();
        }

        {
            std::lock_guard<std::mutex> lock(m_pushDispatchMutex);
            m_pushDispatchQueue.clear();
        }
    }

    m_link.stop();

    m_selfPublicKey.reset();

    {
        std::lock_guard<std::mutex> lock(m_authMutex);
        m_authenticatedPeers.clear();
    }

    {
        std::lock_guard<std::mutex> lock(m_peerMutex);
        m_peerCache.clear();
    }
}

bool MeshCoreClient::isConnected() const
{
    return m_running.load() && m_link.isRunning();
}

void MeshCoreClient::setPushCallback(PushCallback cb)
{
    std::lock_guard<std::mutex> lock(m_cbMutex);
    m_pushCb = std::move(cb);
}

void MeshCoreClient::setMessageCallback(MessageCallback cb)
{
    std::lock_guard<std::mutex> lock(m_cbMutex);
    m_msgCb = std::move(cb);
}

void MeshCoreClient::pushDispatchLoop()
{
    while (true)
    {
        std::pair<uint8_t, std::vector<uint8_t>> item;

        {
            std::unique_lock<std::mutex> lock(m_pushDispatchMutex);
            m_pushDispatchCv.wait(lock, [this]()
            {
                return !m_running.load() || !m_pushDispatchQueue.empty();
            });

            if (!m_running.load()) return;

            item = std::move(m_pushDispatchQueue.front());
            m_pushDispatchQueue.pop_front();
        }

        PushCallback cb;
        {
            std::lock_guard<std::mutex> lock(m_cbMutex);
            cb = m_pushCb;
        }

        if (cb) cb(item.first, item.second);
    }
}

void MeshCoreClient::runForever()
{
    std::unique_lock<std::mutex> lock(m_runMutex);

    while (m_running)
    {
        m_runCv.wait_for(lock, std::chrono::seconds(1));
    }
}

std::optional<uint32_t> MeshCoreClient::getTime()
{
    if (!isConnected())
    {
        return std::nullopt;
    }


    std::lock_guard<std::mutex> apiLock(m_apiMutex);

    std::vector<uint8_t> cmd = { MeshCoreProto::CMD_GET_DEVICE_TIME };
    auto resp = m_link.requestResponse(cmd, MeshCoreProto::RESP_CODE_CURR_TIME, 3000);

    if (!resp.has_value() || resp->size() < 1 + 4)
    {
        return std::nullopt;
    }

    return le32(resp->data() + 1);
}

std::optional<uint32_t> MeshCoreClient::getNodeID()
{
    if (!m_selfPublicKey.has_value())
    {
        if (!doHandshake())
        {
            return std::nullopt;
        }
    }

    if (!m_selfPublicKey.has_value())
    {
        return std::nullopt;
    }

    return be32(m_selfPublicKey->data());
}

std::optional<std::vector<MeshCoreClient::Peer>> MeshCoreClient::listPeers(std::optional<uint32_t> since)
{
    if (!isConnected())
    {
        return std::nullopt;
    }

    // CMD_GET_CONTACTS is a streamed transaction: CONTACTS_START is followed by
    // zero or more CONTACT frames and finally END_OF_CONTACTS. Keep the API lock
    // for the complete stream. Otherwise a second listPeers() or another command
    // can interleave with the active iterator and both callers share/clear the
    // same capture queue.
    std::lock_guard<std::mutex> apiLock(m_apiMutex);

    // Enable capture of CONTACT/END frames coming via link callback
    {
        std::lock_guard<std::mutex> lock(m_captureMutex);
        m_captureQueue.clear();
        m_captureContacts = true;
    }

    std::vector<uint8_t> cmd;
    cmd.push_back(MeshCoreProto::CMD_GET_CONTACTS);

    if (since.has_value())
    {
        uint32_t s = *since;
        cmd.push_back(static_cast<uint8_t>(s & 0xFF));
        cmd.push_back(static_cast<uint8_t>((s >> 8) & 0xFF));
        cmd.push_back(static_cast<uint8_t>((s >> 16) & 0xFF));
        cmd.push_back(static_cast<uint8_t>((s >> 24) & 0xFF));
    }

    std::optional<std::vector<uint8_t>> startResp;
    startResp = m_link.requestResponse(cmd, MeshCoreProto::RESP_CODE_CONTACTS_START, 3000);
    if (!startResp.has_value())
    {
        std::lock_guard<std::mutex> lock(m_captureMutex);
        m_captureContacts = false;
        m_captureQueue.clear();
        return std::nullopt;
    }

    std::vector<Peer> peers;

    auto t0 = std::chrono::steady_clock::now();

    while (true)
    {
        std::vector<uint8_t> frame;

        {
            std::unique_lock<std::mutex> lock(m_captureMutex);

            bool ok = m_captureCv.wait_for(lock, std::chrono::milliseconds(1500), [&]()
            {
                return !m_captureQueue.empty() || !m_running.load();
            });

            if (!m_running)
            {
                m_captureContacts = false;
                m_captureQueue.clear();
                return std::nullopt;
            }

            if (!ok || m_captureQueue.empty())
            {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - t0).count();

                if (elapsed > 8000)
                {
                    m_captureContacts = false;
                    m_captureQueue.clear();
                    return std::nullopt;
                }

                continue;
            }

            frame = std::move(m_captureQueue.front());
            m_captureQueue.pop_front();
        }

        if (frame.empty())
        {
            continue;
        }

        uint8_t code = frame[0];

        if (code == MeshCoreProto::RESP_CODE_END_OF_CONTACTS)
        {
            break;
        }

        if (code != MeshCoreProto::RESP_CODE_CONTACT)
        {
            continue;
        }

        //debugPrintContactFrame(frame);

        MeshCoreProto::ContactRecord rec {};
        if (!MeshCoreProto::decodeContactRecord(frame, rec))
        {
            continue;
        }

        Peer p {};
        p.publicKey = rec.publicKey;
        p.name = rec.name;
        p.lastAdvert = rec.lastAdvert;
        p.lastMod = rec.lastMod;
        p.type = rec.type;
        p.flags = rec.flags;
        p.advLatE6 = rec.advLatE6;
        p.advLonE6 = rec.advLonE6;

        peers.push_back(p);
    }

    {
        std::lock_guard<std::mutex> lock(m_captureMutex);
        m_captureContacts = false;
        m_captureQueue.clear();
    }

    {
        std::lock_guard<std::mutex> lock(m_peerMutex);
        m_peerCache = peers;
    }

    return peers;
}

std::optional<MeshCoreClient::TxQueued> MeshCoreClient::sendMessageEx(uint32_t nodeId, const std::string &text, uint8_t attempt, uint32_t senderTimestamp)
{
    if (!isConnected())
    {
        return std::nullopt;
    }

    // Ensure we have some peer cache; try refreshing once.
    {
        std::lock_guard<std::mutex> lock(m_peerMutex);
        if (m_peerCache.empty())
        {
            // fall through
        }
    }

    if (m_peerCache.empty())
    {
        auto peersOpt = listPeers(std::nullopt);
        if (!peersOpt.has_value())
        {
            return std::nullopt;
        }
    }

    std::optional<std::array<uint8_t, 6>> prefix;

    {
        std::lock_guard<std::mutex> lock(m_peerMutex);

        for (const auto &p : m_peerCache)
        {
            if (p.nodeId() == nodeId)
            {
                prefix = p.prefix6();
                break;
            }
        }
    }

    if (!prefix.has_value())
    {
        return std::nullopt;
    }

    std::string clipped = text;
    if (clipped.size() > 160)
    {
        clipped.resize(160);
    }

    const uint8_t txtType = 0;

    std::vector<uint8_t> cmd;
    cmd.reserve(1 + 1 + 1 + 4 + 6 + clipped.size());

    cmd.push_back(MeshCoreProto::CMD_SEND_TXT_MSG);
    cmd.push_back(txtType);
    cmd.push_back(attempt);

    const uint32_t ts = senderTimestamp;
    cmd.push_back(static_cast<uint8_t>(ts & 0xFF));
    cmd.push_back(static_cast<uint8_t>((ts >> 8) & 0xFF));
    cmd.push_back(static_cast<uint8_t>((ts >> 16) & 0xFF));
    cmd.push_back(static_cast<uint8_t>((ts >> 24) & 0xFF));

    for (size_t i = 0; i < 6; i++)
    {
        cmd.push_back((*prefix)[i]);
    }

    cmd.insert(cmd.end(), clipped.begin(), clipped.end());

    std::optional<std::vector<uint8_t>> resp;
    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponse(cmd, MeshCoreProto::RESP_CODE_SENT, 5000);
    }
    if (!resp.has_value() || resp->size() < 1 + 1 + 4)
    {
        return std::nullopt;
    }

    // RESP_CODE_SENT:
    // [0]=code, [1]=reserved, [2..5]=ack_code (LE), [6..9]=suggested_timeout_ms (LE, optional)
    const uint32_t ack = MeshCoreProto::le32(resp->data() + 2);

    uint32_t suggestedTimeoutMs = 1000;
    if (resp->size() >= 1 + 1 + 4 + 4)
    {
        suggestedTimeoutMs = MeshCoreProto::le32(resp->data() + 6);
    }

    TxQueued out {};
    out.nodeId = nodeId;
    out.ack = ack;
    out.suggestedTimeoutMs = suggestedTimeoutMs;
    return out;
}

std::optional<uint32_t> MeshCoreClient::sendMessage(uint32_t nodeId, const std::string &text)
{
    const uint32_t ts = nowUtcEpoch();
    auto r = sendMessageEx(nodeId, text, 0, ts);
    if (!r.has_value())
    {
        return std::nullopt;
    }
    return r->ack;
}



std::optional<MeshCoreClient::TxQueued> MeshCoreClient::sendMessageToNameEx(const std::string &name, const std::string &text, uint8_t attempt, uint32_t senderTimestamp)
{
    if (!isConnected())
    {
        return std::nullopt;
    }

    auto normalize = [](const std::string &s)
    {
        // Lower-case, strip common control chars, trim.
        std::string out;
        out.reserve(s.size());

        for (unsigned char c : s)
        {
            if (c < 0x20 || c == 0x7F)
            {
                continue;
            }
            out.push_back(static_cast<char>(std::tolower(c)));
        }

        // trim
        size_t a = 0;
        while (a < out.size() && std::isspace(static_cast<unsigned char>(out[a])))
        {
            a++;
        }
        size_t b = out.size();
        while (b > a && std::isspace(static_cast<unsigned char>(out[b - 1])))
        {
            b--;
        }
        return out.substr(a, b - a);
    };

    const std::string needle = normalize(name);

    // Ensure we have a reasonably fresh peer cache.
    bool cacheOk = false;
    {
        std::lock_guard<std::mutex> lock(m_peerMutex);
        cacheOk = !m_peerCache.empty();
    }
    if (!cacheOk)
    {
        auto peersOpt = listPeers(std::nullopt);
        if (!peersOpt.has_value())
        {
            return std::nullopt;
        }
    }

    std::optional<uint32_t> exactId;
    std::optional<uint32_t> partialId;
    int partialCount = 0;

    {
        std::lock_guard<std::mutex> lock(m_peerMutex);

        for (const auto &p : m_peerCache)
        {
            const std::string pname = normalize(p.name);
            const uint32_t id = p.nodeId();

            if (pname == needle)
            {
                exactId = id;
                break;
            }

            if (!needle.empty() && pname.find(needle) != std::string::npos)
            {
                partialId = id;
                partialCount++;
            }
        }
    }

    if (exactId.has_value())
    {
        return sendMessageEx(*exactId, text, attempt, senderTimestamp);
    }

    if (partialId.has_value() && partialCount == 1)
    {
        return sendMessageEx(*partialId, text, attempt, senderTimestamp);
    }

    return std::nullopt;
}

std::optional<uint32_t> MeshCoreClient::sendMessageToName(const std::string &name, const std::string &text)
{
    const uint32_t ts = nowUtcEpoch();
    auto r = sendMessageToNameEx(name, text, 0, ts);
    if (!r.has_value())
    {
        return std::nullopt;
    }
    return r->ack;
}

bool MeshCoreClient::setRadioParams(uint32_t freqHz,
                                    uint32_t bwHz,
                                    uint8_t sf,
                                    uint8_t cr,
                                    bool repeatMode)
{
    if (!isConnected())
    {
        return false;
    }

    const uint32_t radioFreq = freqHz / 1000;
    const uint32_t radioBw = bwHz;

    std::vector<uint8_t> cmd;
    cmd.reserve(1 + 4 + 4 + 1 + 1 + 1);

    cmd.push_back(MeshCoreProto::CMD_SET_RADIO_PARAMS);

    cmd.push_back(static_cast<uint8_t>(radioFreq & 0xFF));
    cmd.push_back(static_cast<uint8_t>((radioFreq >> 8) & 0xFF));
    cmd.push_back(static_cast<uint8_t>((radioFreq >> 16) & 0xFF));
    cmd.push_back(static_cast<uint8_t>((radioFreq >> 24) & 0xFF));

    cmd.push_back(static_cast<uint8_t>(radioBw & 0xFF));
    cmd.push_back(static_cast<uint8_t>((radioBw >> 8) & 0xFF));
    cmd.push_back(static_cast<uint8_t>((radioBw >> 16) & 0xFF));
    cmd.push_back(static_cast<uint8_t>((radioBw >> 24) & 0xFF));

    cmd.push_back(sf);
    cmd.push_back(cr);
    cmd.push_back(repeatMode ? 1 : 0);

    std::optional<std::vector<uint8_t>> resp;

    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponseAny(
            cmd,
            std::vector<uint8_t>
            {
                MeshCoreProto::RESP_CODE_OK,
                MeshCoreProto::RESP_CODE_ERR
            },
            3000
        );
    }

    if (!resp.has_value() || resp->empty())
    {
        return false;
    }

    return ((*resp)[0] == MeshCoreProto::RESP_CODE_OK);
}

bool MeshCoreClient::setPathHashMode(uint8_t mode)
{
    if (!isConnected())
    {
        return false;
    }

    if (mode > 2)
    {
        return false;
    }

    std::vector<uint8_t> cmd;
    cmd.reserve(3);

    cmd.push_back(MeshCoreProto::CMD_SET_PATH_HASH_MODE);
    cmd.push_back(0x00);
    cmd.push_back(mode);

    std::optional<std::vector<uint8_t>> resp;

    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponseAny(
            cmd,
            std::vector<uint8_t>
            {
                MeshCoreProto::RESP_CODE_OK,
                MeshCoreProto::RESP_CODE_ERR
            },
            3000
        );
    }

    if (!resp.has_value() || resp->empty())
    {
        return false;
    }

    return ((*resp)[0] == MeshCoreProto::RESP_CODE_OK);
}

bool MeshCoreClient::sendSelfAdvert(bool flood)
{
    if (!isConnected())
    {
        return false;
    }

    std::vector<uint8_t> cmd;
    cmd.push_back(MeshCoreProto::CMD_SEND_SELF_ADVERT);
    cmd.push_back(flood ? 1 : 0);

    std::optional<std::vector<uint8_t>> resp;
    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponse(cmd, MeshCoreProto::RESP_CODE_OK, 2000);
    }
    return resp.has_value();
}

bool MeshCoreClient::doHandshake()
{
    std::lock_guard<std::mutex> apiLock(m_apiMutex);

    if (!isConnected())
    {
        return false;
    }

    // CMD_DEVICE_QUERY
    {
        uint8_t appTargetVer = 3;
        std::vector<uint8_t> cmd = { MeshCoreProto::CMD_DEVICE_QUERY, appTargetVer };

        auto resp = m_link.requestResponse(cmd, MeshCoreProto::RESP_CODE_DEVICE_INFO, 3000);
        if (!resp.has_value())
        {
            return false;
        }

        // RESP_CODE_DEVICE_INFO:
        // [0] code
        // [1] firmware_ver
        // [2] max_contacts_div_2   (ver 3+)
        // [3] max_channels         (ver 3+)
        if (resp->size() >= 4)
        {
            const uint8_t reportedMaxChannels = (*resp)[3];

            if (reportedMaxChannels > 0)
            {
                m_maxChannels = reportedMaxChannels;
            }
        }
    }

    // CMD_APP_START -> RESP_CODE_SELF_INFO
    {
        uint8_t appVer = 1;
        std::string appName = "my-monitor";

        std::vector<uint8_t> cmd;
        cmd.reserve(1 + 1 + 6 + appName.size());

        cmd.push_back(MeshCoreProto::CMD_APP_START);
        cmd.push_back(appVer);

        for (int i = 0; i < 6; i++)
        {
            cmd.push_back(0x00);
        }

        cmd.insert(cmd.end(), appName.begin(), appName.end());

        auto resp = m_link.requestResponse(cmd, MeshCoreProto::RESP_CODE_SELF_INFO, 3000);
        if (!resp.has_value())
        {
            return false;
        }

        // RESP_CODE_SELF_INFO contains public_key(32) at offset 4:
        // [code][type][tx][max_tx][pubkey32]...
        if (resp->size() >= 1 + 1 + 1 + 1 + 32)
        {
            std::array<uint8_t, 32> pk {};
            size_t pkOff = 1 + 1 + 1 + 1;

            for (size_t i = 0; i < 32; i++)
            {
                pk[i] = (*resp)[pkOff + i];
            }

            m_selfPublicKey = pk;
        }
    }

    return true;
}

void MeshCoreClient::taskLoop()
{
    while (m_running)
    {
        std::unique_lock<std::mutex> lock(m_taskMutex);

        m_taskCv.wait(lock, [&]()
        {
            return !m_running || m_msgSyncPending;
        });

        if (!m_running)
        {
            return;
        }

        m_msgSyncPending = false;
        lock.unlock();

        syncAllMessagesOnce();
    }
}

void MeshCoreClient::triggerMsgSync()
{
    {
        std::lock_guard<std::mutex> lock(m_taskMutex);
        m_msgSyncPending = true;
    }

    m_taskCv.notify_all();
}

void MeshCoreClient::syncAllMessagesOnce()
{
    static constexpr std::size_t MAX_MESSAGES_PER_PASS = 8;
    std::size_t processed = 0;

    while (m_running && isConnected() && processed < MAX_MESSAGES_PER_PASS)
    {
        std::vector<uint8_t> cmd = { MeshCoreProto::CMD_SYNC_NEXT_MESSAGE };
        std::optional<std::vector<uint8_t>> resp;

        // Serialize only the actual Companion command. Decoding, DB/name
        // resolution and the application callback must not hold m_apiMutex.
        {
            std::lock_guard<std::mutex> apiLock(m_apiMutex);
            resp = m_link.requestResponseAny(
                cmd,
                std::vector<uint8_t>
                {
                    MeshCoreProto::RESP_CODE_CONTACT_MSG_RECV_V3,
                    MeshCoreProto::RESP_CODE_CONTACT_MSG_RECV,
                    MeshCoreProto::RESP_CODE_CHANNEL_MSG_RECV_V3,
                    MeshCoreProto::RESP_CODE_CHANNEL_MSG_RECV,
                    MeshCoreProto::RESP_CODE_NO_MORE_MESSAGES
                },
                800
            );
        }

        if (!resp.has_value()) return;
        if (resp->empty()) return;

        const uint8_t code = (*resp)[0];
        if (code == MeshCoreProto::RESP_CODE_NO_MORE_MESSAGES) return;

        auto msgOpt = decodeRxMessage(*resp);
        if (!msgOpt.has_value())
        {
            std::cout << "[MSGSYNC] decodeRxMessage failed\n";
            processed++;
            continue;
        }

        RxMessage msg = *msgOpt;
        std::string fromName;

        if (!msg.isChannel) fromName = nameFromPrefix6(msg.senderPrefix6);
        else fromName = MeshDB::ResolveChannelDisplayName(msg.channelIdx);

        MessageCallback mcb;
        {
            std::lock_guard<std::mutex> lock(m_cbMutex);
            mcb = m_msgCb;
        }

        if (mcb) mcb(msg, fromName);
        processed++;
    }

    // There may be more messages. Requeue instead of monopolizing the
    // Companion API lock; GUI actions and radio requests get a chance first.
    if (m_running && isConnected() && processed >= MAX_MESSAGES_PER_PASS) triggerMsgSync();
}

std::optional<MeshCoreClient::RxMessage> MeshCoreClient::decodeRxMessage(const std::vector<uint8_t> &frame)
{
    if (frame.empty())
    {
        return std::nullopt;
    }

    const uint8_t code = frame[0];

    RxMessage m {};
    m.snrDb = std::nanf("");
    m.respCode = code;
    m.rawFrame = frame;

    if (code == MeshCoreProto::RESP_CODE_CONTACT_MSG_RECV)
    {
        std::cout << "[decodeRxMessage] CONTACT_MSG_RECV legacy\n";

        if (frame.size() < 1 + 6 + 1 + 1 + 4)
        {
            return std::nullopt;
        }

        m.isChannel = false;

        for (size_t i = 0; i < 6; ++i)
        {
            m.senderPrefix6[i] = frame[1 + i];
        }

        m.pathLen = frame[1 + 6];
        m.txtType = frame[1 + 6 + 1];
        m.senderTimestamp = le32(frame.data() + (1 + 6 + 1 + 1));

        const size_t textOff = 1 + 6 + 1 + 1 + 4;
        m.text.assign(reinterpret_cast<const char *>(frame.data() + textOff), frame.size() - textOff);

        return m;
    }

    if (code == MeshCoreProto::RESP_CODE_CONTACT_MSG_RECV_V3)
    {
        if (frame.size() < 1 + 1 + 2 + 6 + 1 + 1 + 4)
        {
            std::cout << "[decodeRxMessage] CONTACT_MSG_RECV_V3 too short: " << frame.size() << "\n";
            return std::nullopt;
        }

        /*std::cout << "raw Frame:\n";
        for (size_t i = 0; i < frame.size(); ++i)
        {
            std::cout
                << std::hex
                << std::setw(2)
                << std::setfill('0')
                << static_cast<int>(frame[i])
                << ' ';
        }
        std::cout << std::dec << std::endl;*/

        m.isChannel = false;

        const uint8_t rawSnr = frame[1];
        const int8_t snr4 = static_cast<int8_t>(rawSnr);
        m.snrDb = static_cast<float>(snr4) / 4.0f;

        /*std::cout << "[decodeRxMessage] CONTACT_MSG_RECV_V3\n";
        std::cout << "  rawSnr           : 0x"
                  << std::hex
                  << std::setw(2)
                  << std::setfill('0')
                  << static_cast<unsigned>(rawSnr)
                  << std::dec
                  << " (" << static_cast<unsigned>(rawSnr) << ")\n";
        std::cout << "  snr4             : " << static_cast<int>(snr4) << "\n";
        std::cout << "  snrDb            : " << m.snrDb << "\n";
        std::cout << "  reserved[2]      : "
                  << std::hex
                  << std::setw(2)
                  << std::setfill('0')
                  << static_cast<unsigned>(frame[2])
                  << " "
                  << std::setw(2)
                  << std::setfill('0')
                  << static_cast<unsigned>(frame[3])
                  << std::dec
                  << "\n";
        */
        

        const size_t pfxOff = 1 + 1 + 2;

        for (size_t i = 0; i < 6; ++i)
        {
            m.senderPrefix6[i] = frame[pfxOff + i];
        }

        const uint8_t pathMeta = frame[pfxOff + 6];
        m.pathLen = pathMeta & 0x3F;
        if(m.pathLen == 0x3F) m.pathLen = 0;

        uint8_t pathHashSizeCode = (pathMeta >> 6) & 0x03;
        m.pathHashSize = static_cast<uint8_t>(pathHashSizeCode + 1);

        m.txtType = frame[pfxOff + 6 + 1];
        m.senderTimestamp = le32(frame.data() + (pfxOff + 6 + 1 + 1));

        const size_t textOff = pfxOff + 6 + 1 + 1 + 4;
        m.text.assign(reinterpret_cast<const char *>(frame.data() + textOff), frame.size() - textOff);

        /*std::cout   << "  pathMeta-Debugausgabe    : "
                    << "frame.size=" << frame.size()
                    << " pfxOff=" << unsigned(pfxOff)
                    << " idx=" << unsigned(pfxOff + 6)
                    << " pathMeta=0x"
                    << std::hex << unsigned(pathMeta)
                    << std::dec
                    << " pathLen=" << unsigned(pathMeta & 0x3F)
                    << "\n";

        std::cout << "  senderPrefix6    : ";
        for (size_t i = 0; i < 6; ++i)
        {
            std::cout << std::hex
                      << std::setw(2)
                      << std::setfill('0')
                      << static_cast<unsigned>(m.senderPrefix6[i]);
        }
        std::cout << std::dec << "\n";

        std::cout << "  pathMeta         : 0x"
                  << std::hex
                  << std::setw(2)
                  << std::setfill('0')
                  << static_cast<unsigned>(pathMeta)
                  << std::dec
                  << " (" << static_cast<unsigned>(pathMeta) << ")\n";
        std::cout << "  pathLen          : " << static_cast<unsigned>(m.pathLen) << "\n";
        std::cout << "  pathHashSizeCode : " << static_cast<unsigned>((pathMeta >> 6) & 0x03) << "\n";
        std::cout << "  pathHashSize     : " << static_cast<unsigned>(((pathMeta >> 6) & 0x03) + 1) << "\n";
        std::cout << "  txtType          : " << static_cast<unsigned>(m.txtType) << "\n";
        std::cout << "  senderTimestamp  : " << m.senderTimestamp << "\n";
        std::cout << "  textLen          : " << m.text.size() << "\n";
        */
        return m;
    }

    if (code == MeshCoreProto::RESP_CODE_CHANNEL_MSG_RECV)
    {
        std::cout << "[decodeRxMessage] CHANNEL_MSG_RECV legacy\n";

        if (frame.size() < 1 + 1 + 1 + 1 + 4)
        {
            return std::nullopt;
        }

        m.isChannel = true;
        m.channelIdx = frame[1];
        m.pathLen = frame[2];
        m.txtType = frame[3];
        m.senderTimestamp = le32(frame.data() + 4);

        const size_t textOff = 1 + 1 + 1 + 1 + 4;
        m.text.assign(reinterpret_cast<const char *>(frame.data() + textOff), frame.size() - textOff);

        return m;
    }

    if (code == MeshCoreProto::RESP_CODE_CHANNEL_MSG_RECV_V3)
    {
        if (frame.size() < 1 + 1 + 2 + 1 + 1 + 1 + 4)
        {
            std::cout << "[decodeRxMessage] CHANNEL_MSG_RECV_V3 too short: "
                      << frame.size() << "\n";
            return std::nullopt;
        }

        m.isChannel = true;

        const uint8_t rawSnr = frame[1];
        const int8_t snr4 = static_cast<int8_t>(rawSnr);
        m.snrDb = static_cast<float>(snr4) / 4.0f;
/*
        std::cout << "[decodeRxMessage] CHANNEL_MSG_RECV_V3\n";
        std::cout << "  rawSnr           : 0x"
                  << std::hex
                  << std::setw(2)
                  << std::setfill('0')
                  << static_cast<unsigned>(rawSnr)
                  << std::dec
                  << " (" << static_cast<unsigned>(rawSnr) << ")\n";
        std::cout << "  snr4             : " << static_cast<int>(snr4) << "\n";
        std::cout << "  snrDb            : " << m.snrDb << "\n";
        std::cout << "  reserved[2]      : "
                  << std::hex
                  << std::setw(2)
                  << std::setfill('0')
                  << static_cast<unsigned>(frame[2])
                  << " "
                  << std::setw(2)
                  << std::setfill('0')
                  << static_cast<unsigned>(frame[3])
                  << std::dec
                  << "\n";
*/
        const size_t off = 1 + 1 + 2;

        m.channelIdx = frame[off + 0];

        const uint8_t pathMeta = frame[off + 1];
        m.pathLen = pathMeta & 0x3F;
        if(m.pathLen == 0x3F) m.pathLen = 0;

        // Optional, falls im Struct vorhanden:
        uint8_t pathHashSizeCode = (pathMeta >> 6) & 0x03;
        m.pathHashSize = static_cast<uint8_t>(pathHashSizeCode + 1);

        m.txtType = frame[off + 2];
        m.senderTimestamp = le32(frame.data() + (off + 3));

        const size_t textOff = off + 3 + 4;
        m.text.assign(reinterpret_cast<const char *>(frame.data() + textOff), frame.size() - textOff);
/*
        std::cout << "  channelIdx       : " << static_cast<unsigned>(m.channelIdx) << "\n";
        std::cout << "  pathMeta         : 0x"
                  << std::hex
                  << std::setw(2)
                  << std::setfill('0')
                  << static_cast<unsigned>(pathMeta)
                  << std::dec
                  << " (" << static_cast<unsigned>(pathMeta) << ")\n";
        std::cout << "  pathLen          : " << static_cast<unsigned>(m.pathLen) << "\n";
        std::cout << "  pathHashSizeCode : " << static_cast<unsigned>((pathMeta >> 6) & 0x03) << "\n";
        std::cout << "  pathHashSize     : " << static_cast<unsigned>(((pathMeta >> 6) & 0x03) + 1) << "\n";
        std::cout << "  txtType          : " << static_cast<unsigned>(m.txtType) << "\n";
        std::cout << "  senderTimestamp  : " << m.senderTimestamp << "\n";
        std::cout << "  textLen          : " << m.text.size() << "\n";
*/
        return m;
    }

    return std::nullopt;
}
void MeshCoreClient::onLinkFrame(uint8_t code, const std::vector<uint8_t> &payload)
{
    // Binary responses are asynchronous pushes. Capture them independently of
    // MeshCoreLink's single request waiter so a fast 0x8C cannot be lost in the
    // small gap after RESP_CODE_SENT and before the caller starts waiting.
    if (code == MeshCoreProto::PUSH_CODE_BINARY_RESPONSE)
    {
        bool captured = false;

        {
            std::lock_guard<std::mutex> lock(m_binaryMutex);

            if (m_binaryCapture.active)
            {
                m_binaryCapture.frames.push_back(payload);
                while (m_binaryCapture.frames.size() > 16) m_binaryCapture.frames.pop_front();
                captured = true;
            }
        }

        if (captured) m_binaryCv.notify_all();
    }

    if (code == MeshCoreProto::PUSH_CODE_PATH_UPDATED ||
        code == MeshCoreProto::PUSH_CODE_LOGIN_SUCCESS ||
        code == MeshCoreProto::PUSH_CODE_LOGIN_FAIL)
    {
        bool loginActive = false;

        {
            std::lock_guard<std::mutex> lock(m_loginMutex);
            loginActive = m_loginCapture.active;
        }

        if (loginActive)
        {
            std::cout
                << "[login-rx] code=0x"
                << std::hex
                << std::setw(2)
                << std::setfill('0')
                << static_cast<unsigned>(code)
                << std::dec
                << " len="
                << payload.size()
                << " raw="
                << BytesToHex(payload)
                << "\n";
        }
    }

    // Synchronous repeater-login capture. A captured login frame is consumed here
    // so the room-auth state machine does not interpret it as a room login.
    if ((code == MeshCoreProto::PUSH_CODE_LOGIN_SUCCESS ||
         code == MeshCoreProto::PUSH_CODE_LOGIN_FAIL) &&
        payload.size() >= 8)
    {
        bool captured = false;

        {
            std::lock_guard<std::mutex> lock(m_loginMutex);

            if (m_loginCapture.active &&
                std::equal(
                    m_loginCapture.prefix.begin(),
                    m_loginCapture.prefix.end(),
                    payload.begin() + 2))
            {
                m_loginCapture.frame = payload;
                m_loginCapture.ready = true;
                m_loginCapture.active = false;
                captured = true;
            }
        }

        if (captured)
        {
            m_loginCv.notify_all();
            return;
        }
    }

    // 1a) contact-stream capture for listPeers()
    {
        std::lock_guard<std::mutex> lock(m_captureMutex);

        if (m_captureContacts && (code == MeshCoreProto::RESP_CODE_CONTACT || code == MeshCoreProto::RESP_CODE_END_OF_CONTACTS))
        {
            m_captureQueue.push_back(payload);
            m_captureCv.notify_all();
            // Don't return; user might want to see frames too.
        }
    }

    // 1b) discover capture
    {
        std::lock_guard<std::mutex> lock(m_discoverMutex);

        if (m_discoverCapture.active &&
            (code == MeshCoreProto::PUSH_CODE_RX_LOG_DATA || code == MeshCoreProto::PUSH_CODE_CONTROL_DATA))
        {
            MeshCoreProto::DiscoverNode dn {};

            if (MeshCoreProto::decodeDiscoverResponse(payload, dn) &&
                dn.tag == m_discoverCapture.tag)
            {
                m_discoverCapture.frames.push_back(payload);
                m_discoverCv.notify_all();
            }
        }
    }

    // 2) update peer cache on new advert (contact-like record)
    if (code == MeshCoreProto::PUSH_CODE_NEW_ADVERT)
    {
        MeshCoreProto::ContactRecord rec {};
        if (MeshCoreProto::decodeContactRecord(payload, rec))
        {
            Peer p {};
            p.publicKey = rec.publicKey;
            p.name = rec.name;
            p.lastAdvert = rec.lastAdvert;
            p.lastMod = rec.lastMod;

            std::lock_guard<std::mutex> lock(m_peerMutex);

            bool updated = false;

            for (auto &existing : m_peerCache)
            {
                if (existing.prefix6() == p.prefix6())
                {
                    existing = p;
                    updated = true;
                    break;
                }
            }

            if (!updated)
            {
                m_peerCache.push_back(p);
            }
        }
    }

    // 3) built-in task trigger
    if (code == MeshCoreProto::PUSH_CODE_MSG_WAITING)
    {
        triggerMsgSync();
    }

    // 4) User/application push processing is deliberately asynchronous.
    // PushRouter writes to MariaDB and may block on MeshDB::s_mutex for seconds.
    // Never let that stall the link RX thread, otherwise protocol responses
    // (LOGIN_SUCCESS, binary responses, stats, ...) remain unread in the socket.
    {
        std::lock_guard<std::mutex> lock(m_pushDispatchMutex);
        m_pushDispatchQueue.emplace_back(code, payload);
    }
    m_pushDispatchCv.notify_one();
}

uint32_t MeshCoreClient::Peer::nodeId() const
{
    return MeshCoreClient::be32(publicKey.data());
}

std::array<uint8_t, 6> MeshCoreClient::Peer::prefix6() const
{
    std::array<uint8_t, 6> p {};
    for (size_t i = 0; i < 6; i++)
    {
        p[i] = publicKey[i];
    }
    return p;
}

std::string MeshCoreClient::nameFromPrefix6(const std::array<uint8_t, 6> &pfx) const
{
    std::lock_guard<std::mutex> lock(m_peerMutex);

    for (const auto &p : m_peerCache)
    {
        if (p.prefix6() == pfx)
        {
            if (!p.name.empty())
            {
                return p.name;
            }
        }
    }

    return "<unknown>";
}

std::string MeshCoreClient::resolveNameFromPrefix6(const std::array<uint8_t, 6> &pfx) const
{
    return nameFromPrefix6(pfx);
}

uint32_t MeshCoreClient::le32(const uint8_t *p)
{
    return MeshCoreProto::le32(p);
}

uint32_t MeshCoreClient::be32(const uint8_t *p)
{
    return MeshCoreProto::be32(p);
}

uint32_t MeshCoreClient::nowUtcEpoch()
{
    return static_cast<uint32_t>(::time(nullptr));
}

bool MeshCoreClient::setManualAddContacts(bool enable)
{
    if (!isConnected())
    {
        return false;
    }

    std::vector<uint8_t> cmd;
    cmd.push_back(MeshCoreProto::CMD_SET_OTHER_PARAMS);
    cmd.push_back(enable ? 1 : 0);

    std::optional<std::vector<uint8_t>> resp;
    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponse(cmd, MeshCoreProto::RESP_CODE_OK, 2000);
    }
    return resp.has_value();
}

std::optional<uint32_t> MeshCoreClient::loginToRoomEx(
    uint32_t roomNodeId,
    const std::string& password)
{
    if (!isConnected())
    {
        return std::nullopt;
    }

    std::optional<std::array<uint8_t, 32>> publicKey;

    {
        std::lock_guard<std::mutex> lock(m_peerMutex);

        for (const auto& p : m_peerCache)
        {
            if (p.nodeId() == roomNodeId)
            {
                publicKey = p.publicKey;
                break;
            }
        }
    }

    if (!publicKey.has_value())
    {
        return std::nullopt;
    }

    std::string clipped = password;

    if (clipped.size() > 15)
    {
        clipped.resize(15);
    }

    std::vector<uint8_t> cmd;
    cmd.reserve(1 + 32 + clipped.size());

    cmd.push_back(MeshCoreProto::CMD_SEND_LOGIN);
    cmd.insert(cmd.end(), publicKey->begin(), publicKey->end());
    cmd.insert(cmd.end(), clipped.begin(), clipped.end());

    std::optional<std::vector<uint8_t>> resp;

    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponse(cmd, MeshCoreProto::RESP_CODE_SENT, 5000);
    }

    if (!resp.has_value())
    {
        return std::nullopt;
    }

    if (resp->size() < 1 + 1 + 4)
    {
        return std::nullopt;
    }

    const uint32_t ack = MeshCoreProto::le32(resp->data() + 2);
    return ack;
}

std::optional<MeshCoreClient::TxQueued> MeshCoreClient::sendRoomMessageEx(
    uint32_t roomNodeId,
    const std::string& text,
    uint8_t attempt,
    uint32_t senderTimestamp)
{
    return sendMessageEx(roomNodeId, text, attempt, senderTimestamp);
}

std::optional<MeshCoreClient::TxQueued> MeshCoreClient::sendChannelMessageEx(
    uint8_t channelIdx,
    const std::string& text,
    uint8_t attempt,
    uint32_t senderTimestamp)
{
    (void)attempt; // Channel-TX kennt im Companion-Protokoll kein attempt-Feld.

    if (!isConnected())
    {
        return std::nullopt;
    }

    std::string clipped = text;

    // Laut Protokoll:
    // max length = 160 - len(advert_name) - 2
    //
    // Die aktuelle Advert-Name-Laenge kennen wir hier nicht sicher.
    // Konservativ auf 160 begrenzen; falls du den lokalen advert_name
    // im Client hast, solltest du hier genauer clippen.
    if (clipped.size() > 160)
    {
        clipped.resize(160);
    }

    const uint8_t txtType = 0; // TXT_TYPE_PLAIN

    std::vector<uint8_t> cmd;
    cmd.reserve(1 + 1 + 1 + 4 + clipped.size());

    cmd.push_back(MeshCoreProto::CMD_SEND_CHANNEL_TXT_MSG);
    cmd.push_back(txtType);
    cmd.push_back(channelIdx);

    const uint32_t ts = senderTimestamp;
    cmd.push_back(static_cast<uint8_t>(ts & 0xFF));
    cmd.push_back(static_cast<uint8_t>((ts >> 8) & 0xFF));
    cmd.push_back(static_cast<uint8_t>((ts >> 16) & 0xFF));
    cmd.push_back(static_cast<uint8_t>((ts >> 24) & 0xFF));

    cmd.insert(cmd.end(), clipped.begin(), clipped.end());

    std::optional<std::vector<uint8_t>> resp;
    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponse(cmd, MeshCoreProto::RESP_CODE_OK, 3000);
    }

    if (!resp.has_value())
    {
        return std::nullopt;
    }

    TxQueued out {};
    out.nodeId = 0;               // Channel-Broadcast, kein Ziel-Node
    out.ack = 0;                  // Protokoll liefert keinen ACK-Code
    out.suggestedTimeoutMs = 0;   // Protokoll liefert keinen Timeout
    return out;
}

std::optional<MeshCoreClient::ChannelInfo> MeshCoreClient::getChannelInfo(uint8_t channelIdx)
{
    if (!isConnected())
    {
        return std::nullopt;
    }

    std::vector<uint8_t> cmd;
    cmd.reserve(2);
    cmd.push_back(MeshCoreProto::CMD_GET_CHANNEL_INFO);
    cmd.push_back(channelIdx);

    std::optional<std::vector<uint8_t>> resp;

    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponse(
            cmd,
            MeshCoreProto::RESP_CODE_CHANNEL_INFO,
            3000);
    }

    if (!resp.has_value())
    {
        return std::nullopt;
    }

    const std::vector<uint8_t>& frame = *resp;

    if (frame.size() < 2 + 16)
    {
        return std::nullopt;
    }

    ChannelInfo out {};
    out.channelIdx = frame[1];

    size_t secretOff = frame.size() - 16;

    for (size_t i = 0; i < 16; i++)
    {
        out.secret[i] = frame[secretOff + i];
    }

    size_t nameOff = 2;
    size_t nameMaxLen = secretOff - nameOff;

    for (size_t i = 0; i < nameMaxLen; i++)
    {
        uint8_t c = frame[nameOff + i];

        if (c == 0)
        {
            break;
        }

        out.name.push_back(static_cast<char>(c));
    }

    return out;
}

bool MeshCoreClient::setChannel(
    uint8_t channelIdx,
    const std::string& name,
    const std::array<uint8_t, 16>& secret)
{
    if (!isConnected())
    {
        return false;
    }

    std::vector<uint8_t> cmd;
    cmd.reserve(1 + 1 + 32 + 16);

    cmd.push_back(MeshCoreProto::CMD_SET_CHANNEL);
    cmd.push_back(channelIdx);

    char nameBuf[32] = {0};
    const size_t copyLen = std::min<size_t>(name.size(), sizeof(nameBuf) - 1);
    std::memcpy(nameBuf, name.data(), copyLen);
    cmd.insert(cmd.end(), nameBuf, nameBuf + sizeof(nameBuf));

    cmd.insert(cmd.end(), secret.begin(), secret.end());

    std::optional<std::vector<uint8_t>> resp;

    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponse(
            cmd,
            MeshCoreProto::RESP_CODE_OK,
            3000);
    }

    return resp.has_value();
}

bool MeshCoreClient::deleteChannel(uint8_t channelIdx)
{
    std::array<uint8_t, 16> zeroSecret {};
    return setChannel(channelIdx, "", zeroSecret);
}

uint8_t MeshCoreClient::maxChannels() const
{
    return m_maxChannels;
}

bool MeshCoreClient::setAdvertName(const std::string& name)
{
    if (!isConnected())
    {
        return false;
    }

    std::string clipped = name;

    if (clipped.size() > 63)
    {
        clipped.resize(63);
    }

    std::vector<uint8_t> cmd;
    cmd.reserve(1 + clipped.size());

    cmd.push_back(MeshCoreProto::CMD_SET_ADVERT_NAME);
    cmd.insert(cmd.end(), clipped.begin(), clipped.end());

    std::optional<std::vector<uint8_t>> resp;

    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponse(cmd, MeshCoreProto::RESP_CODE_OK, 3000);
    }

    return resp.has_value();
}

bool MeshCoreClient::setAdvertLocation(int32_t latitudeE6, int32_t longitudeE6)
{
    if (!isConnected())
    {
        return false;
    }

    std::vector<uint8_t> cmd;
    cmd.reserve(1 + 4 + 4);

    cmd.push_back(MeshCoreProto::CMD_SET_ADVERT_LATLON);

    cmd.push_back(static_cast<uint8_t>(latitudeE6 & 0xFF));
    cmd.push_back(static_cast<uint8_t>((latitudeE6 >> 8) & 0xFF));
    cmd.push_back(static_cast<uint8_t>((latitudeE6 >> 16) & 0xFF));
    cmd.push_back(static_cast<uint8_t>((latitudeE6 >> 24) & 0xFF));

    cmd.push_back(static_cast<uint8_t>(longitudeE6 & 0xFF));
    cmd.push_back(static_cast<uint8_t>((longitudeE6 >> 8) & 0xFF));
    cmd.push_back(static_cast<uint8_t>((longitudeE6 >> 16) & 0xFF));
    cmd.push_back(static_cast<uint8_t>((longitudeE6 >> 24) & 0xFF));

    std::optional<std::vector<uint8_t>> resp;

    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponseAny(
            cmd,
            std::vector<uint8_t>
            {
                MeshCoreProto::RESP_CODE_OK,
                MeshCoreProto::RESP_CODE_ERR
            },
            3000
        );
    }

    if (!resp.has_value() || resp->empty())
    {
        return false;
    }

    return ((*resp)[0] == MeshCoreProto::RESP_CODE_OK);
}

bool MeshCoreClient::setRadioTxPower(uint8_t txPowerDbm)
{
    if (!isConnected())
    {
        return false;
    }

    std::vector<uint8_t> cmd;
    cmd.reserve(1 + 1);

    cmd.push_back(MeshCoreProto::CMD_SET_RADIO_TX_POWER);
    cmd.push_back(txPowerDbm);

    std::optional<std::vector<uint8_t>> resp;

    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponseAny(
            cmd,
            std::vector<uint8_t>
            {
                MeshCoreProto::RESP_CODE_OK,
                MeshCoreProto::RESP_CODE_ERR
            },
            3000
        );
    }

    if (!resp.has_value() || resp->empty())
    {
        return false;
    }

    return ((*resp)[0] == MeshCoreProto::RESP_CODE_OK);
}

std::optional<std::vector<MeshCoreClient::DiscoverResult>> MeshCoreClient::discoverRepeaters(
    int ackTimeoutMs,
    int settleMs,
    int maxTotalMs)
{
    return discoverNodes(
        MeshCoreProto::DISCOVER_FILTER_REPEATER,
        ackTimeoutMs,
        settleMs,
        maxTotalMs);
}

std::optional<std::vector<MeshCoreClient::DiscoverResult>> MeshCoreClient::discoverNodes(
    uint8_t typeFilter,
    int ackTimeoutMs,
    int settleMs,
    int maxTotalMs)
{
    if (!isConnected())
    {
        std::cout << "[discover] not connected\n";
        return std::nullopt;
    }

    std::lock_guard<std::mutex> apiLock(m_apiMutex);

    const uint32_t tag =
        static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count());

    {
        std::lock_guard<std::mutex> lock(m_discoverMutex);
        m_discoverCapture.active = true;
        m_discoverCapture.tag = tag;
        m_discoverCapture.frames.clear();
    }

    std::vector<uint8_t> cmd;
    cmd.reserve(1 + 1 + 1 + 4);
    cmd.push_back(MeshCoreProto::CMD_SEND_CONTROL_DATA);
    cmd.push_back(MeshCoreProto::CONTROL_OP_DISCOVER);
    cmd.push_back(typeFilter);
    cmd.push_back(static_cast<uint8_t>(tag & 0xFF));
    cmd.push_back(static_cast<uint8_t>((tag >> 8) & 0xFF));
    cmd.push_back(static_cast<uint8_t>((tag >> 16) & 0xFF));
    cmd.push_back(static_cast<uint8_t>((tag >> 24) & 0xFF));
/*
    std::cout
        << "[discover] TX CMD_SEND_CONTROL_DATA opcode=0x81 filter=0x"
        << std::hex << static_cast<unsigned>(typeFilter)
        << " tag=0x" << tag
        << std::dec
        << " raw=" << bytesToHexVec(cmd).c_str()
        << "\n";
*/
    auto ack = m_link.requestResponse(cmd, MeshCoreProto::RESP_CODE_OK, ackTimeoutMs);

    if (!ack.has_value())
    {
        std::cout << "[discover] no ACK / no RESP_CODE_OK\n";

        std::lock_guard<std::mutex> lock(m_discoverMutex);
        m_discoverCapture.active = false;
        m_discoverCapture.frames.clear();
        return std::nullopt;
    }

    // std::cout << "[discover] ACK OK raw=" << bytesToHexVec(*ack).c_str() << "\n";

    std::map<std::string, DiscoverResult> collected;

    const auto tStart = std::chrono::steady_clock::now();
    auto tLastHit = tStart;

    while (true)
    {
        std::vector<uint8_t> frame;
        bool gotFrame = false;

        {
            std::unique_lock<std::mutex> lock(m_discoverMutex);

            m_discoverCv.wait_for(lock, std::chrono::milliseconds(200), [&]()
            {
                return !m_discoverCapture.frames.empty() || !m_running.load();
            });

            if (!m_running.load())
            {
                break;
            }

            if (!m_discoverCapture.frames.empty())
            {
                frame = std::move(m_discoverCapture.frames.front());
                m_discoverCapture.frames.pop_front();
                gotFrame = true;
            }
        }

        const auto now = std::chrono::steady_clock::now();
        const auto totalMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - tStart).count();
        const auto idleMs = std::chrono::duration_cast<std::chrono::milliseconds>(now - tLastHit).count();

        if (gotFrame)
        {
            tLastHit = now;

            std::cout << "[discover] RX raw=" << bytesToHexVec(frame).c_str() << "\n";

            MeshCoreProto::DiscoverNode dn {};
            if (!MeshCoreProto::decodeDiscoverResponse(frame, dn))
            {
                std::cout << "[discover] frame ignored (decode failed)\n";
                continue;
            }

            DiscoverResult r {};
            r.nodeId = dn.nodeId;
            r.snrRxDb = dn.snrRxDb;
            r.snrTxDb = dn.snrTxDb;
            r.rssiDbm = dn.rssiDbm;
            r.sourceCode = dn.sourceCode;

            const std::string key = nodeIdKey(r.nodeId);

            auto it = collected.find(key);

            if (it == collected.end())
            {
                collected.emplace(key, r);
/*
                std::cout
                    << "[discover] node="
                    << nodeId8ToHex(r.nodeId)
                    << " snr_rx=" << r.snrRxDb
                    << " snr_tx=" << r.snrTxDb
                    << " rssi=" << r.rssiDbm
                    << " source=0x" << std::hex << static_cast<unsigned>(r.sourceCode) << std::dec
                    << "\n";
*/                    
            }
            else
            {
                if (ShouldReplaceDiscoverResult(it->second, r))
                {
                    it->second = r;

                    if (m_enableRxLog.load())
                    {
/*                        
                        std::cout
                            << "[discover] node="
                            << nodeId8ToHex(r.nodeId)
                            << " updated from source=0x"
                            << std::hex << static_cast<unsigned>(r.sourceCode)
                            << std::dec
                            << "\n";
*/                            
                    }
                }
                else
                {
                    if (m_enableRxLog.load())
                    {
                        std::cout
                            << "[discover] duplicate node "
                            << nodeId8ToHex(r.nodeId)
                            << " skipped\n";
                    }
                }
            }
        }

        if (totalMs >= maxTotalMs)
        {
            std::cout << "[discover] stop: maxTotalMs reached\n";
            break;
        }

        if (!gotFrame && idleMs >= settleMs)
        {
            std::cout << "[discover] stop: settle timeout reached\n";
            break;
        }
    }

    {
        std::lock_guard<std::mutex> lock(m_discoverMutex);
        m_discoverCapture.active = false;
        m_discoverCapture.frames.clear();
    }

    std::vector<DiscoverResult> out;
    out.reserve(collected.size());

    for (const auto& entry : collected)
    {
        out.push_back(entry.second);
    }

    //std::cout << "[discover] done, " << out.size() << " unique nodes found\n";

    return out;
}

bool MeshCoreClient::setTime(uint32_t epochSecsUtc)
{
    if (!isConnected())
    {
        return false;
    }

    std::vector<uint8_t> cmd;
    cmd.reserve(1 + 4);
    cmd.push_back(MeshCoreProto::CMD_SET_DEVICE_TIME);
    cmd.push_back(static_cast<uint8_t>(epochSecsUtc & 0xFF));
    cmd.push_back(static_cast<uint8_t>((epochSecsUtc >> 8) & 0xFF));
    cmd.push_back(static_cast<uint8_t>((epochSecsUtc >> 16) & 0xFF));
    cmd.push_back(static_cast<uint8_t>((epochSecsUtc >> 24) & 0xFF));

    std::optional<std::vector<uint8_t>> resp;
    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponse(cmd, MeshCoreProto::RESP_CODE_OK, 3000);
    }

    return resp.has_value();
}

bool MeshCoreClient::syncClock()
{
    return setTime(nowUtcEpoch());
}


bool MeshCoreClient::addOrUpdateContact(const Peer& peer)
{
    if (!isConnected()) return false;
    if (peer.name.empty()) return false;

    static constexpr uint8_t OUT_PATH_UNKNOWN = 0xFF;
    static constexpr size_t MAX_PATH_SIZE = 64;
    static constexpr size_t CONTACT_NAME_SIZE = 32;

    std::vector<uint8_t> cmd;
    cmd.reserve(1 + 32 + 1 + 1 + 1 + MAX_PATH_SIZE + CONTACT_NAME_SIZE + 4 + 8);

    cmd.push_back(MeshCoreProto::CMD_ADD_UPDATE_CONTACT);
    cmd.insert(cmd.end(), peer.publicKey.begin(), peer.publicKey.end());
    cmd.push_back(peer.type);
    cmd.push_back(peer.flags);
    cmd.push_back(OUT_PATH_UNKNOWN);
    cmd.insert(cmd.end(), MAX_PATH_SIZE, 0);

    std::array<uint8_t, CONTACT_NAME_SIZE> nameField {};
    const size_t nameLen = std::min(peer.name.size(), CONTACT_NAME_SIZE - 1);
    std::memcpy(nameField.data(), peer.name.data(), nameLen);
    cmd.insert(cmd.end(), nameField.begin(), nameField.end());

    auto appendLe32 = [&cmd](uint32_t value)
    {
        cmd.push_back(static_cast<uint8_t>(value & 0xFF));
        cmd.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
        cmd.push_back(static_cast<uint8_t>((value >> 16) & 0xFF));
        cmd.push_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    };

    appendLe32(peer.lastAdvert);
    appendLe32(static_cast<uint32_t>(peer.advLatE6));
    appendLe32(static_cast<uint32_t>(peer.advLonE6));

    std::optional<std::vector<uint8_t>> resp;

    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponseAny(
            cmd,
            std::vector<uint8_t>
            {
                MeshCoreProto::RESP_CODE_OK,
                MeshCoreProto::RESP_CODE_ERR
            },
            3000);
    }

    if (!resp.has_value() || resp->empty()) return false;

    if ((*resp)[0] == MeshCoreProto::RESP_CODE_ERR)
    {
        std::cerr << "[contacts] add/update failed for \""
                  << peer.name
                  << "\"";

        if (resp->size() >= 2) std::cerr << " err=" << unsigned((*resp)[1]);
        std::cerr << "\n";
        return false;
    }

    std::cout << "[contacts] added/updated companion contact: \""
              << peer.name
              << "\" with unknown path (flood)\n";

    return true;
}

bool MeshCoreClient::loginToPeerSync(const Peer& peer, const std::string& password)
{
    if (!isConnected()) return false;

    std::string clipped = password;
    if (clipped.size() > 15) clipped.resize(15);

    const auto prefix = peer.prefix6();

    {
        std::lock_guard<std::mutex> lock(m_loginMutex);
        if (m_loginCapture.active || m_loginCapture.ready) return false;

        m_loginCapture.active = true;
        m_loginCapture.ready = false;
        m_loginCapture.prefix = prefix;
        m_loginCapture.frame.clear();
    }

    auto cancelCapture = [this]()
    {
        std::lock_guard<std::mutex> lock(m_loginMutex);
        m_loginCapture.active = false;
        m_loginCapture.ready = false;
        m_loginCapture.frame.clear();
    };

    std::vector<uint8_t> cmd;
    cmd.reserve(1 + peer.publicKey.size() + clipped.size());
    cmd.push_back(MeshCoreProto::CMD_SEND_LOGIN);
    cmd.insert(cmd.end(), peer.publicKey.begin(), peer.publicKey.end());
    cmd.insert(cmd.end(), clipped.begin(), clipped.end());

    std::cout
        << "[login] sending repeater login to "
        << peer.name
        << " passwordLen="
        << clipped.size()
        << std::endl;

    std::optional<std::vector<uint8_t>> sentResp;
    uint32_t waitMs = 15000;
    std::vector<uint8_t> frame;

    // Keep the companion API serialized for the whole login transaction.
    // Companion firmware has only one pending login/request slot; another
    // outgoing request can clear it before the RF response arrives.
    std::unique_lock<std::mutex> apiLock(m_apiMutex);

    sentResp = m_link.requestResponseAny(
        cmd,
        std::vector<uint8_t>
        {
            MeshCoreProto::RESP_CODE_SENT,
            MeshCoreProto::RESP_CODE_ERR
        },
        5000);

    if (!sentResp.has_value() || sentResp->empty())
    {
        apiLock.unlock();
        cancelCapture();
        std::cerr << "[login] companion did not accept login command\n";
        return false;
    }

    if ((*sentResp)[0] == MeshCoreProto::RESP_CODE_ERR)
    {
        const unsigned err = sentResp->size() >= 2 ? (*sentResp)[1] : 0xFF;
        apiLock.unlock();
        cancelCapture();
        std::cerr << "[login] companion rejected login command, err=" << err << "\n";
        return false;
    }

    if (sentResp->size() >= 10)
    {
        const uint32_t suggested = MeshCoreProto::le32(sentResp->data() + 6);
        waitMs = std::clamp<uint32_t>(suggested + 3000U, 15000U, 30000U);
    }

    if (sentResp->size() >= 6)
    {
        const uint32_t tag = MeshCoreProto::le32(sentResp->data() + 2);
        std::cout
            << "[login] queued tag=0x"
            << std::hex
            << std::setw(8)
            << std::setfill('0')
            << tag
            << std::dec
            << " waitMs="
            << waitMs
            << std::endl;
    }

    const auto waitStarted = std::chrono::steady_clock::now();

    std::cout
        << "[login] entering response wait clientRunning="
        << (m_running.load() ? "yes" : "no")
        << " linkRunning="
        << (m_link.isRunning() ? "yes" : "no")
        << std::endl;

    {
        std::unique_lock<std::mutex> loginLock(m_loginMutex);
        const bool gotReply = m_loginCv.wait_for(
            loginLock,
            std::chrono::milliseconds(waitMs),
            [this]()
            {
                return m_loginCapture.ready;
            });

        const auto elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - waitStarted).count();

        std::cout
            << "[login] response wait finished elapsedMs="
            << elapsedMs
            << " ready="
            << (m_loginCapture.ready ? "yes" : "no")
            << " clientRunning="
            << (m_running.load() ? "yes" : "no")
            << " linkRunning="
            << (m_link.isRunning() ? "yes" : "no")
            << std::endl;

        if (!gotReply || !m_loginCapture.ready)
        {
            m_loginCapture.active = false;
            m_loginCapture.ready = false;
            m_loginCapture.frame.clear();
            apiLock.unlock();
            std::cerr << "[login] no login response from " << peer.name << "\n";
            return false;
        }

        frame = std::move(m_loginCapture.frame);
        m_loginCapture.active = false;
        m_loginCapture.ready = false;
    }

    apiLock.unlock();

    if (frame.empty()) return false;

    if (frame[0] == MeshCoreProto::PUSH_CODE_LOGIN_FAIL)
    {
        std::cerr << "[login] login failed for " << peer.name << "\n";
        return false;
    }

    if (frame[0] != MeshCoreProto::PUSH_CODE_LOGIN_SUCCESS || frame.size() < 8)
    {
        std::cerr << "[login] unexpected login response for " << peer.name << "\n";
        return false;
    }

    const bool isAdmin = (frame[1] & 0x01) != 0;

    std::cout
        << "[login] success for "
        << peer.name
        << " admin="
        << (isAdmin ? "yes" : "no");

    if (frame.size() >= 13)
    {
        std::cout
            << " acl_permissions=0x"
            << std::hex
            << std::setw(2)
            << std::setfill('0')
            << static_cast<unsigned>(frame[12])
            << std::dec;
    }

    std::cout << "\n";

    {
        std::lock_guard<std::mutex> lock(m_authMutex);
        m_authenticatedPeers.insert(prefix);
    }

    return true;
}

std::optional<std::string> MeshCoreClient::requestNeighboursRaw(const std::string& repeaterName)
{
    if (repeaterName.empty())
    {
        std::cerr << "[neighbours] empty repeater name\n";
        return std::nullopt;
    }

    auto peersOpt = listPeers(std::nullopt);
    if (!peersOpt.has_value())
    {
        std::cerr << "[neighbours] could not fetch contacts\n";
        return std::nullopt;
    }

    for (const auto& peer : *peersOpt)
    {
        if (peer.name == repeaterName) return requestNeighboursRaw(peer);
    }

    std::cerr << "[neighbours] repeater not found: " << repeaterName << "\n";
    return std::nullopt;
}

std::optional<std::string> MeshCoreClient::requestNeighboursRaw(const Peer& repeater, const std::string& password)
{
    static constexpr uint8_t binaryReqTypeNeighbours = 0x00;
    static constexpr uint8_t neighbourCount = 0xFF;
    static constexpr uint8_t pubKeyPrefixLength = 0x04;
    static constexpr size_t responseHeaderSize = 4;
    static constexpr size_t neighbourRecordSize = 9;
    static constexpr unsigned MAX_LOGIN_ATTEMPTS = 5;
    static constexpr unsigned MAX_ATTEMPTS_PER_PAGE = 5;
    static constexpr uint32_t LOGIN_RETRY_DELAY_MS = 500;
    static constexpr uint32_t RETRY_DELAY_MS = 500;

    if (!isConnected())
    {
        std::cerr << "[neighbours] not connected\n";
        return std::nullopt;
    }

    const auto prefix = repeater.prefix6();

    if (!password.empty())
    {
        bool authenticated = false;
        {
            std::lock_guard<std::mutex> lock(m_authMutex);
            authenticated = m_authenticatedPeers.find(prefix) != m_authenticatedPeers.end();
        }

        if (!authenticated)
        {
            for (unsigned attempt = 1; attempt <= MAX_LOGIN_ATTEMPTS; ++attempt)
            {
                std::cout
                    << "[login] attempt="
                    << attempt
                    << "/"
                    << MAX_LOGIN_ATTEMPTS
                    << " for "
                    << repeater.name
                    << std::endl;

                if (loginToPeerSync(repeater, password))
                {
                    authenticated = true;
                    break;
                }

                if (attempt < MAX_LOGIN_ATTEMPTS)
                {
                    std::cout
                        << "[login] retrying "
                        << repeater.name
                        << " in "
                        << LOGIN_RETRY_DELAY_MS
                        << "ms"
                        << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(LOGIN_RETRY_DELAY_MS));
                }
            }

            if (!authenticated)
            {
                std::cerr
                    << "[neighbours] repeater login failed after "
                    << MAX_LOGIN_ATTEMPTS
                    << " attempts: "
                    << repeater.name
                    << "\n";
                return std::nullopt;
            }
        }
        else
        {
            std::cout << "[login] already authenticated in this session, skipping login for "
                      << repeater.name << std::endl;
        }
    }

    const auto readLe16 = [](const uint8_t* p) -> uint16_t
    {
        return static_cast<uint16_t>(p[0]) |
               (static_cast<uint16_t>(p[1]) << 8);
    };

    const auto readLe32Local = [](const uint8_t* p) -> uint32_t
    {
        return static_cast<uint32_t>(p[0]) |
               (static_cast<uint32_t>(p[1]) << 8) |
               (static_cast<uint32_t>(p[2]) << 16) |
               (static_cast<uint32_t>(p[3]) << 24);
    };

    const auto apiWaitStart = std::chrono::steady_clock::now();
    std::unique_lock<std::mutex> apiLock(m_apiMutex);
    const auto apiWaitMs = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - apiWaitStart).count();
    if (apiWaitMs > 50) std::cout << "[neighbours] waited " << apiWaitMs << "ms for companion API lock" << std::endl;

    std::vector<std::array<uint8_t, neighbourRecordSize>> mergedRecords;
    uint32_t offset = 0;
    uint16_t reportedTotal = 0;
    unsigned pageNumber = 0;

    while (pageNumber == 0 || offset < reportedTotal)
    {
        pageNumber++;
        bool pageComplete = false;
        uint16_t pageTotal = 0;
        uint16_t pageResultCount = 0;
        std::vector<uint8_t> pageData;

        for (unsigned attempt = 1; attempt <= MAX_ATTEMPTS_PER_PAGE; attempt++)
        {
            const uint32_t randomBlob = static_cast<uint32_t>(
                std::chrono::steady_clock::now().time_since_epoch().count());

            std::vector<uint8_t> requestData;
            requestData.reserve(10);
            requestData.push_back(binaryReqTypeNeighbours);
            requestData.push_back(neighbourCount);
            requestData.push_back(static_cast<uint8_t>(offset & 0xFF));
            requestData.push_back(static_cast<uint8_t>((offset >> 8) & 0xFF));
            requestData.push_back(0x00); // order_by: newest -> oldest
            requestData.push_back(pubKeyPrefixLength);
            requestData.push_back(static_cast<uint8_t>(randomBlob & 0xFF));
            requestData.push_back(static_cast<uint8_t>((randomBlob >> 8) & 0xFF));
            requestData.push_back(static_cast<uint8_t>((randomBlob >> 16) & 0xFF));
            requestData.push_back(static_cast<uint8_t>((randomBlob >> 24) & 0xFF));

            std::vector<uint8_t> cmd;
            cmd.reserve(1 + repeater.publicKey.size() + 1 + requestData.size());
            cmd.push_back(MeshCoreProto::CMD_SEND_RAW_DATA);
            cmd.insert(cmd.end(), repeater.publicKey.begin(), repeater.publicKey.end());
            cmd.push_back(0x06); // REQ_TYPE_GET_NEIGHBOURS
            cmd.insert(cmd.end(), requestData.begin(), requestData.end());

            std::cout
                << "[neighbours] requesting page=" << pageNumber
                << " offset=" << offset
                << " from " << repeater.name
                << " attempt=" << attempt << "/" << MAX_ATTEMPTS_PER_PAGE
                << "\n"
                << "[neighbours] binary request data: "
                << BytesToHex(requestData)
                << "\n"
                << "[neighbours] companion raw command: "
                << BytesToHex(cmd)
                << std::endl;

            {
                std::lock_guard<std::mutex> lock(m_binaryMutex);
                m_binaryCapture.active = true;
                m_binaryCapture.frames.clear();
            }

            auto cancelBinaryCapture = [this]()
            {
                std::lock_guard<std::mutex> lock(m_binaryMutex);
                m_binaryCapture.active = false;
                m_binaryCapture.frames.clear();
            };

            auto sentResp = m_link.requestResponse(
                cmd,
                MeshCoreProto::RESP_CODE_SENT,
                5000);

            if (!sentResp.has_value() || sentResp->size() < 6)
            {
                cancelBinaryCapture();
                std::cerr << "[neighbours] no sent ack response page=" << pageNumber
                          << " offset=" << offset
                          << " attempt=" << attempt << "/" << MAX_ATTEMPTS_PER_PAGE
                          << "\n";
                if (attempt < MAX_ATTEMPTS_PER_PAGE) std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
                continue;
            }

            const uint32_t responseTag = MeshCoreProto::le32(sentResp->data() + 2);
            const bool sentFlood = sentResp->size() >= 2 && (*sentResp)[1] != 0;
            const uint32_t suggestedMs = sentResp->size() >= 10
                ? MeshCoreProto::le32(sentResp->data() + 6)
                : 5000U;
            const uint32_t waitMs = std::clamp<uint32_t>(suggestedMs + 3000U, 6000U, 15000U);

            std::cout
                << "[neighbours] sent ack/tag: 0x"
                << std::hex
                << std::setw(8)
                << std::setfill('0')
                << responseTag
                << std::dec
                << " page=" << pageNumber
                << " offset=" << offset
                << " mode=" << (sentFlood ? "flood" : "direct")
                << " suggestedMs=" << suggestedMs
                << " waitMs=" << waitMs
                << std::endl;

            std::optional<std::vector<uint8_t>> resp;
            const auto responseWaitStarted = std::chrono::steady_clock::now();

            {
                std::unique_lock<std::mutex> lock(m_binaryMutex);

                const auto findResponse = [this, responseTag]()
                {
                    return std::find_if(
                        m_binaryCapture.frames.begin(),
                        m_binaryCapture.frames.end(),
                        [responseTag](const std::vector<uint8_t>& frame)
                        {
                            if (frame.size() < 6) return false;
                            return MeshCoreProto::le32(frame.data() + 2) == responseTag;
                        });
                };

                const bool got = m_binaryCv.wait_for(
                    lock,
                    std::chrono::milliseconds(waitMs),
                    [&]()
                    {
                        return findResponse() != m_binaryCapture.frames.end() || !m_running.load();
                    });

                if (got)
                {
                    auto it = findResponse();
                    if (it != m_binaryCapture.frames.end())
                    {
                        resp = std::move(*it);
                        m_binaryCapture.frames.erase(it);
                    }
                }

                m_binaryCapture.active = false;
                m_binaryCapture.frames.clear();
            }

            const auto responseWaitMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - responseWaitStarted).count();

            std::cout
                << "[neighbours] binary response wait finished elapsedMs="
                << responseWaitMs
                << " got=" << (resp.has_value() ? "yes" : "no")
                << " page=" << pageNumber
                << " offset=" << offset
                << std::endl;

            if (!resp.has_value() || resp->size() < 6)
            {
                std::cerr << "[neighbours] no binary response for sent ack/tag 0x"
                          << std::hex
                          << std::setw(8)
                          << std::setfill('0')
                          << responseTag
                          << std::dec
                          << " page=" << pageNumber
                          << " offset=" << offset
                          << " attempt=" << attempt << "/" << MAX_ATTEMPTS_PER_PAGE
                          << "\n";
                if (attempt < MAX_ATTEMPTS_PER_PAGE) std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
                continue;
            }

            pageData.assign(resp->begin() + 6, resp->end());

            if (pageData.size() < responseHeaderSize)
            {
                std::cerr << "[neighbours] short page response page=" << pageNumber
                          << " offset=" << offset
                          << " attempt=" << attempt << "/" << MAX_ATTEMPTS_PER_PAGE
                          << " bytes=" << pageData.size() << "\n";
                if (attempt < MAX_ATTEMPTS_PER_PAGE) std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
                continue;
            }

            pageTotal = readLe16(pageData.data());
            pageResultCount = readLe16(pageData.data() + 2);
            const size_t neededSize = responseHeaderSize +
                                      static_cast<size_t>(pageResultCount) * neighbourRecordSize;

            if (pageData.size() < neededSize)
            {
                std::cerr << "[neighbours] truncated page response page=" << pageNumber
                          << " offset=" << offset
                          << " total=" << pageTotal
                          << " result_count=" << pageResultCount
                          << " bytes=" << pageData.size()
                          << " needed=" << neededSize
                          << " attempt=" << attempt << "/" << MAX_ATTEMPTS_PER_PAGE
                          << "\n";
                if (attempt < MAX_ATTEMPTS_PER_PAGE) std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
                continue;
            }

            if (pageResultCount == 0 && offset < pageTotal)
            {
                std::cerr << "[neighbours] empty page before end page=" << pageNumber
                          << " offset=" << offset
                          << " total=" << pageTotal
                          << " attempt=" << attempt << "/" << MAX_ATTEMPTS_PER_PAGE
                          << "\n";
                if (attempt < MAX_ATTEMPTS_PER_PAGE) std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
                continue;
            }

            std::cout
                << "[neighbours] page received page=" << pageNumber
                << " offset=" << offset
                << " total=" << pageTotal
                << " result_count=" << pageResultCount
                << " attempt=" << attempt << "/" << MAX_ATTEMPTS_PER_PAGE
                << std::endl;

            pageComplete = true;
            break;
        }

        if (!pageComplete)
        {
            std::cerr << "[neighbours] page failed after " << MAX_ATTEMPTS_PER_PAGE
                      << " attempts page=" << pageNumber
                      << " offset=" << offset << "\n";

            if (!password.empty())
            {
                std::lock_guard<std::mutex> lock(m_authMutex);
                m_authenticatedPeers.erase(prefix);
            }

            return std::nullopt;
        }

        if (pageTotal > reportedTotal) reportedTotal = pageTotal;

        for (uint16_t i = 0; i < pageResultCount; i++)
        {
            const size_t pos = responseHeaderSize + static_cast<size_t>(i) * neighbourRecordSize;
            std::array<uint8_t, neighbourRecordSize> record {};
            std::copy_n(pageData.begin() + static_cast<std::ptrdiff_t>(pos), neighbourRecordSize, record.begin());

            auto existing = std::find_if(
                mergedRecords.begin(),
                mergedRecords.end(),
                [&record](const std::array<uint8_t, neighbourRecordSize>& candidate)
                {
                    return std::equal(record.begin(), record.begin() + 4, candidate.begin());
                });

            if (existing == mergedRecords.end())
            {
                mergedRecords.push_back(record);
                continue;
            }

            const uint32_t oldSecsAgo = readLe32Local(existing->data() + 4);
            const uint32_t newSecsAgo = readLe32Local(record.data() + 4);
            if (newSecsAgo < oldSecsAgo) *existing = record;
        }

        std::cout
            << "[neighbours] aggregate=" << mergedRecords.size()
            << "/" << reportedTotal
            << " after page=" << pageNumber
            << std::endl;

        if (pageResultCount == 0) break;
        offset += pageResultCount;

        if (offset > 0xFFFFU)
        {
            std::cerr << "[neighbours] offset exceeds protocol limit: " << offset << "\n";
            return std::nullopt;
        }
    }

    std::vector<uint8_t> combined;
    combined.reserve(responseHeaderSize + mergedRecords.size() * neighbourRecordSize);
    combined.push_back(static_cast<uint8_t>(reportedTotal & 0xFF));
    combined.push_back(static_cast<uint8_t>((reportedTotal >> 8) & 0xFF));

    const uint16_t combinedCount = static_cast<uint16_t>(
        std::min<size_t>(mergedRecords.size(), 0xFFFFU));
    combined.push_back(static_cast<uint8_t>(combinedCount & 0xFF));
    combined.push_back(static_cast<uint8_t>((combinedCount >> 8) & 0xFF));

    for (const auto& record : mergedRecords)
    {
        combined.insert(combined.end(), record.begin(), record.end());
    }

    std::cout
        << "[neighbours] complete pages=" << pageNumber
        << " total=" << reportedTotal
        << " result_count=" << combinedCount
        << " raw_bytes=" << combined.size()
        << std::endl;

    if (combinedCount < reportedTotal)
    {
        std::cerr << "[neighbours] warning: collected " << combinedCount
                  << " unique neighbours but repeater reported " << reportedTotal
                  << "\n";
    }

    return BytesToHex(combined);
}

bool MeshCoreClient::resetPath(const std::array<uint8_t, 32>& publicKey)
{
    if (!isConnected())
    {
        return false;
    }

    std::vector<uint8_t> cmd;
    cmd.reserve(1 + publicKey.size());

    cmd.push_back(MeshCoreProto::CMD_RESET_PATH);
    cmd.insert(cmd.end(), publicKey.begin(), publicKey.end());

    std::optional<std::vector<uint8_t>> resp;

    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponseAny(
            cmd,
            std::vector<uint8_t>
            {
                MeshCoreProto::RESP_CODE_OK,
                MeshCoreProto::RESP_CODE_ERR
            },
            3000
        );
    }

    if (!resp.has_value() || resp->empty())
    {
        return false;
    }

    return ((*resp)[0] == MeshCoreProto::RESP_CODE_OK);
}

bool MeshCoreClient::removeContact(const std::array<uint8_t, 32>& publicKey)
{
    if (!isConnected())
    {
        return false;
    }

    std::vector<uint8_t> cmd;
    cmd.reserve(1 + publicKey.size());

    cmd.push_back(MeshCoreProto::CMD_REMOVE_CONTACT);
    cmd.insert(cmd.end(), publicKey.begin(), publicKey.end());

    std::optional<std::vector<uint8_t>> resp;

    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponseAny(
            cmd,
            std::vector<uint8_t>
            {
                MeshCoreProto::RESP_CODE_OK,
                MeshCoreProto::RESP_CODE_ERR
            },
            3000
        );
    }

    if (!resp.has_value() || resp->empty())
    {
        return false;
    }

    return ((*resp)[0] == MeshCoreProto::RESP_CODE_OK);
}

std::optional<MeshCoreClient::RadioStats> MeshCoreClient::getRadioStats()
{
    if (!isConnected())
    {
        return std::nullopt;
    }

    std::vector<uint8_t> cmd;
    cmd.reserve(2);
    cmd.push_back(MeshCoreProto::CMD_GET_STATS_RADIO);
    cmd.push_back(0x01);

    std::optional<std::vector<uint8_t>> resp;

    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponseMatching(
            cmd,
            std::vector<uint8_t> { MeshCoreProto::RESP_CODE_STATS_RADIO },
            [](const std::vector<uint8_t>& frame)
            {
                return frame.size() >= 2 && frame[1] == 0x01;
            },
            1000
        );
    }

    if (!resp.has_value())
    {
        std::cerr << "[radio_status] no response for stats_radio command\n";
        return std::nullopt;
    }

    if (resp->size() < 14)
    {
        std::cerr << "[radio_status] response too short, size="
                << resp->size()
                << "\n";
        return std::nullopt;
    }

    const std::vector<uint8_t>& frame = *resp;

    if (frame[0] != MeshCoreProto::RESP_CODE_STATS_RADIO || frame[1] != 0x01)
    {
        
        std::cerr << "[radio_status] unexpected response: code=0x"
                << std::hex << static_cast<int>(frame[0])
                << " sub=0x"
                << static_cast<int>(frame[1])
                << std::dec << "\n";

        return std::nullopt;
    }

    RadioStats stats {};
    stats.noiseFloor = static_cast<int8_t>(frame[2]);
    stats.lastRssi = static_cast<int8_t>(frame[4]);
    stats.lastSnr = static_cast<float>(static_cast<int8_t>(frame[5])) / 4.0f;
    stats.txAirSecs = le32(frame.data() + 6);
    stats.rxAirSecs = le32(frame.data() + 10);

    return stats;
}

std::optional<MeshCoreClient::CoreStats> MeshCoreClient::getCoreStats()
{
    if (!isConnected())
    {
        return std::nullopt;
    }

    std::vector<uint8_t> cmd;
    cmd.reserve(2);
    cmd.push_back(MeshCoreProto::CMD_GET_STATS_RADIO);
    cmd.push_back(0x00);

    std::optional<std::vector<uint8_t>> resp;

    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        resp = m_link.requestResponseMatching(
            cmd,
            std::vector<uint8_t> { MeshCoreProto::RESP_CODE_STATS_RADIO },
            [](const std::vector<uint8_t>& frame)
            {
                return frame.size() >= 2 && frame[1] == 0x00;
            },
            1000
        );
    }

    if (!resp.has_value())
    {
        std::cerr << "[radio_status] no response for stats_core command\n";
        return std::nullopt;
    }

    if (resp->size() < 11)
    {
        std::cerr << "[radio_status] stats_core response too short, size="
                  << resp->size()
                  << "\n";

        return std::nullopt;
    }

    const std::vector<uint8_t>& frame = *resp;

    if (frame[0] != MeshCoreProto::RESP_CODE_STATS_RADIO || frame[1] != 0x00)
    {
        std::cerr << "[radio_status] unexpected stats_core response: code=0x"
                  << std::hex << static_cast<int>(frame[0])
                  << " sub=0x"
                  << static_cast<int>(frame[1])
                  << std::dec << "\n";

        return std::nullopt;
    }

    CoreStats stats {};
    stats.batteryMv = static_cast<uint16_t>(frame[2] | (frame[3] << 8));
    stats.uptimeSecs = le32(frame.data() + 4);
    stats.errors = static_cast<uint16_t>(frame[8] | (frame[9] << 8));
    stats.queueLen = frame[10];

    return stats;
}