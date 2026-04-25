#pragma once

#include <array>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <deque>
#include <functional>
#include <mutex>
#include <optional>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <cmath>

#include "MeshCoreLink.h"
#include "MeshCoreProto.h"

class MeshCoreClient
{
public:
    struct Peer
    {
        std::array<uint8_t, 32> publicKey;
        std::string name;
        uint32_t lastAdvert; // epoch secs (UTC) from contact record
        uint32_t lastMod;    // epoch secs (UTC) from contact record
        uint8_t type = 0;
        uint8_t flags = 0;
        int32_t advLatE6 = 0;
        int32_t advLonE6 = 0;

        uint32_t nodeId() const;
        std::array<uint8_t, 6> prefix6() const;
    };

    struct RxMessage
    {
        bool isChannel;
        uint8_t channelIdx = 0;
        uint8_t txtType = 0;
        uint8_t pathLen = 0;
        float snrDb = nanf("");
        uint8_t pathHashSize;
        uint32_t senderTimestamp = 0;
        std::array<uint8_t, 6> senderPrefix6 {};
        std::string text;

        uint8_t respCode = 0;
        std::vector<uint8_t> rawFrame;
    };

    struct ChannelInfo
    {
        uint8_t channelIdx = 0;
        std::string name;
        std::array<uint8_t, 16> secret {};
        bool isEmpty() const
        {
            if (!name.empty())
            {
                return false;
            }

            for (uint8_t b : secret)
            {
                if (b != 0)
                {
                    return false;
                }
            }

            return true;
        }
    };

    struct DiscoverResult
    {
        std::array<uint8_t, 8> nodeId {};
        float snrRxDb = 0.0f;
        float snrTxDb = 0.0f;
        int rssiDbm = 0;
        uint8_t sourceCode = 0;
    };

    struct RadioStats
    {
        int noiseFloor = 0;
        int lastRssi = 0;
        float lastSnr = 0.0f;
        uint32_t txAirSecs = 0;
        uint32_t rxAirSecs = 0;
    };

    std::optional<RadioStats> getRadioStats();

    std::optional<std::vector<DiscoverResult>> discoverRepeaters(
        int ackTimeoutMs = 3000,
        int settleMs = 1500,
        int maxTotalMs = 5000);

    std::optional<std::vector<DiscoverResult>> discoverNodes(
        uint8_t typeFilter,
        int ackTimeoutMs,
        int settleMs,
        int maxTotalMs);

    using PushCallback = std::function<void(uint8_t code, const std::vector<uint8_t> &payload)>;
    using MessageCallback = std::function<void(const RxMessage &msg, const std::string &fromName)>;

public:
    MeshCoreClient();
    ~MeshCoreClient();

    bool connect(const std::string &port);
    void disconnect();

    bool isConnected() const;

    void setPushCallback(PushCallback cb);
    void setMessageCallback(MessageCallback cb);

    // Keep running: block caller thread until disconnect() (or CTRL+C in main).
    void runForever();

    // Basic
    std::optional<uint32_t> getTime();   // epoch secs (UTC)
    bool setTime(uint32_t epochSecsUtc);
    bool syncClock();
    std::optional<uint32_t> getNodeID(); // derived from self public key (first 4 bytes, BE)

    // Contacts
    std::optional<std::vector<Peer>> listPeers(std::optional<uint32_t> since = std::nullopt);

    bool resetPath(const std::array<uint8_t, 32>& publicKey);

    // Messaging
    struct TxQueued
    {
        uint32_t nodeId;
        uint32_t ack;
        uint32_t suggestedTimeoutMs;
    };

    // Non-blocking send. 'attempt' should be 0..3. Keep senderTimestamp constant across retries.
    std::optional<TxQueued> sendMessageEx(uint32_t nodeId, const std::string &text, uint8_t attempt, uint32_t senderTimestamp);
    std::optional<TxQueued> sendMessageToNameEx(const std::string &name, const std::string &text, uint8_t attempt, uint32_t senderTimestamp);

    // Convenience wrappers (attempt=0, senderTimestamp=now)
    std::optional<uint32_t> sendMessage(uint32_t nodeId, const std::string &text);
    std::optional<uint32_t> sendMessageToName(const std::string &name, const std::string &text);
// Radio
    bool setRadioParams(uint32_t freqHz,
                        uint32_t bwHz,
                        uint8_t sf,
                        uint8_t cr,
                        bool repeatMode = false);

    bool sendSelfAdvert(bool flood = false);

    std::string nameFromPrefix6(const std::array<uint8_t, 6> &pfx) const;

    bool setManualAddContacts(bool enable);

    // Helper für main(): Prefix -> Name (oder "<unknown>")
    std::string resolveNameFromPrefix6(const std::array<uint8_t, 6> &pfx) const;

    bool setAdvertName(const std::string& name);
    bool setAdvertLocation(int32_t latitudeE6, int32_t longitudeE6);
    bool setRadioTxPower(uint8_t txPowerDbm);

    static uint32_t le32(const uint8_t *p);
    static uint32_t be32(const uint8_t *p);

    std::optional<uint32_t> loginToRoomEx(
                        uint32_t roomNodeId,
                        const std::string& password);

    std::optional<TxQueued> sendRoomMessageEx(
                        uint32_t roomNodeId,
                        const std::string& text,
                        uint8_t attempt,
                        uint32_t senderTimestamp);

    std::optional<TxQueued> sendChannelMessageEx(
        uint8_t channelIdx,
        const std::string& text,
        uint8_t attempt,
        uint32_t senderTimestamp);

    std::optional<ChannelInfo> getChannelInfo(uint8_t channelIdx);

    bool setChannel(
        uint8_t channelIdx,
        const std::string& name,
        const std::array<uint8_t, 16>& secret);

    bool deleteChannel(uint8_t channelIdx);

    uint8_t maxChannels() const;

    std::optional<std::vector<uint8_t>> requestResponseAny(
        const std::vector<uint8_t> &cmdPayload,
        const std::vector<uint8_t> &wantedCodes,
        int timeoutMs);
        
private:
    std::atomic<bool> m_running;
    uint8_t m_maxChannels = 40;

    // Link layer (Serial + RX thread + single outstanding requestResponse)
    MeshCoreLink m_link;

    // Threads
    std::thread m_taskThread;

    // Serialize request/response API calls (MeshCoreLink supports one outstanding request)
    mutable std::mutex m_apiMutex;

    // Callbacks
    mutable std::mutex m_cbMutex;
    PushCallback m_pushCb;
    MessageCallback m_msgCb;

    // Cached self identity
    std::optional<std::array<uint8_t, 32>> m_selfPublicKey;

    // Peer cache (for prefix->name resolution)
    mutable std::mutex m_peerMutex;
    std::vector<Peer> m_peerCache;

    // Task signaling (message sync)
    std::mutex m_taskMutex;
    std::condition_variable m_taskCv;
    bool m_msgSyncPending;

    // runForever() support
    std::mutex m_runMutex;
    std::condition_variable m_runCv;

    std::atomic<bool> m_enableRxLog;

    // Contact-stream capture for listPeers()
    std::mutex m_captureMutex;
    std::condition_variable m_captureCv;
    bool m_captureContacts;
    std::deque<std::vector<uint8_t>> m_captureQueue;

private:
    bool doHandshake();

    // Threads
    void taskLoop();

    // Message queue
    void triggerMsgSync();
    void syncAllMessagesOnce();

    static std::optional<RxMessage> decodeRxMessage(const std::vector<uint8_t> &frame);

    // Internal push handler from MeshCoreLink
    void onLinkFrame(uint8_t code, const std::vector<uint8_t> &payload);

    static uint32_t nowUtcEpoch();

        struct DiscoverCapture
    {
        bool active = false;
        uint32_t tag = 0;
        std::deque<std::vector<uint8_t>> frames;
    };

    std::mutex m_discoverMutex;
    std::condition_variable m_discoverCv;
    DiscoverCapture m_discoverCapture;
};