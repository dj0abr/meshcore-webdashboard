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

    m_running = true;

    // Link callback: internal handling first, then user callback
    m_link.setPushCallback([this](uint8_t code, const std::vector<uint8_t> &payload)
    {
        onLinkFrame(code, payload);
    });

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

        {
            std::lock_guard<std::mutex> lock(m_captureMutex);
            m_captureContacts = false;
            m_captureQueue.clear();
        }
        m_captureCv.notify_all();

        if (m_taskThread.joinable())
        {
            m_taskThread.join();
        }
    }

    m_link.stop();

    m_selfPublicKey.reset();

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
    {
        std::lock_guard<std::mutex> apiLock(m_apiMutex);
        startResp = m_link.requestResponse(cmd, MeshCoreProto::RESP_CODE_CONTACTS_START, 3000);
    }
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
    std::lock_guard<std::mutex> apiLock(m_apiMutex);

    while (m_running && isConnected())
    {
        std::vector<uint8_t> cmd = { MeshCoreProto::CMD_SYNC_NEXT_MESSAGE };

        auto resp = m_link.requestResponseAny(
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

        if (!resp.has_value())
        {
            //std::cout << "[MSGSYNC] no response\n";
            return;
        }

        if (resp->empty())
        {
            //std::cout << "[MSGSYNC] empty response\n";
            return;
        }

        const uint8_t code = (*resp)[0];

        /*std::cout << "[MSGSYNC] raw message frame\n";
        std::cout << "  code             : 0x"
                  << std::hex
                  << std::setw(2)
                  << std::setfill('0')
                  << static_cast<unsigned>(code)
                  << std::dec
                  << " (" << RespCodeToString(code) << ")\n";
        std::cout << "  frameLen         : " << resp->size() << "\n";
        std::cout << "  frameHex         : " << BytesToHex(*resp) << "\n";
        DumpFrameBytes(*resp);*/

        if (code == MeshCoreProto::RESP_CODE_NO_MORE_MESSAGES)
        {
            //std::cout << "[MSGSYNC] no more messages\n";
            return;
        }

        auto msgOpt = decodeRxMessage(*resp);

        if (!msgOpt.has_value())
        {
            std::cout << "[MSGSYNC] decodeRxMessage failed\n";
            continue;
        }

        RxMessage msg = *msgOpt;

        //std::cout << "[MSGSYNC] decoded message summary\n";
        //DumpDecodedMessage(msg);

        std::string fromName;
        if (!msg.isChannel)
        {
            fromName = nameFromPrefix6(msg.senderPrefix6);
        }
        else
        {
            fromName = MeshDB::ResolveChannelDisplayName(msg.channelIdx);
        }

        //std::cout << "  resolvedFromName : [" << fromName << "]\n";

        MessageCallback mcb;
        {
            std::lock_guard<std::mutex> lock(m_cbMutex);
            mcb = m_msgCb;
        }

        if (!mcb)
        {
        }
        else
        {
            mcb(msg, fromName);
        }
    }
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

        uint8_t pathHashSizeCode = (pathMeta >> 6) & 0x03;
        m.pathHashSize = static_cast<uint8_t>(pathHashSizeCode + 1);

        m.txtType = frame[pfxOff + 6 + 1];
        m.senderTimestamp = le32(frame.data() + (pfxOff + 6 + 1 + 1));

        const size_t textOff = pfxOff + 6 + 1 + 1 + 4;
        m.text.assign(reinterpret_cast<const char *>(frame.data() + textOff), frame.size() - textOff);

        /*std::cout << "  senderPrefix6    : ";
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

    // 4) user push callback
    PushCallback cb;
    {
        std::lock_guard<std::mutex> lock(m_cbMutex);
        cb = m_pushCb;
    }

    if (cb)
    {
        cb(code, payload);
    }
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

    std::cout
        << "[discover] TX CMD_SEND_CONTROL_DATA opcode=0x81 filter=0x"
        << std::hex << static_cast<unsigned>(typeFilter)
        << " tag=0x" << tag
        << std::dec
        << " raw=" << bytesToHexVec(cmd).c_str()
        << "\n";

    auto ack = m_link.requestResponse(cmd, MeshCoreProto::RESP_CODE_OK, ackTimeoutMs);

    if (!ack.has_value())
    {
        std::cout << "[discover] no ACK / no RESP_CODE_OK\n";

        std::lock_guard<std::mutex> lock(m_discoverMutex);
        m_discoverCapture.active = false;
        m_discoverCapture.frames.clear();
        return std::nullopt;
    }

    std::cout << "[discover] ACK OK raw=" << bytesToHexVec(*ack).c_str() << "\n";

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

                std::cout
                    << "[discover] node="
                    << nodeId8ToHex(r.nodeId)
                    << " snr_rx=" << r.snrRxDb
                    << " snr_tx=" << r.snrTxDb
                    << " rssi=" << r.rssiDbm
                    << " source=0x" << std::hex << static_cast<unsigned>(r.sourceCode) << std::dec
                    << "\n";
            }
            else
            {
                if (ShouldReplaceDiscoverResult(it->second, r))
                {
                    it->second = r;

                    if (m_enableRxLog.load())
                    {
                        std::cout
                            << "[discover] node="
                            << nodeId8ToHex(r.nodeId)
                            << " updated from source=0x"
                            << std::hex << static_cast<unsigned>(r.sourceCode)
                            << std::dec
                            << "\n";
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

    std::cout << "[discover] done, " << out.size() << " unique nodes found\n";

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
        resp = m_link.requestResponseAny(
            cmd,
            std::vector<uint8_t>
            {
                MeshCoreProto::RESP_CODE_STATS_RADIO
            },
            3000
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
