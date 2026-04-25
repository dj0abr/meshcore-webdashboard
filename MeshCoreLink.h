#pragma once

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

#include "SerialPort.h"
#include "MeshCoreFramer.h"
#include "IByteStream.h"
#include "TcpPort.h"

#include <memory>

class MeshCoreLink
{
public:
    using PushCallback = std::function<void(uint8_t code, const std::vector<uint8_t> &payload)>;

public:
    MeshCoreLink();
    ~MeshCoreLink();

    bool start(const std::string &device);
    void stop();

    bool isRunning() const;

    void setPushCallback(PushCallback cb);

    std::optional<std::vector<uint8_t>> requestResponse(const std::vector<uint8_t> &cmdPayload,
                                                        uint8_t wantedCode,
                                                        int timeoutMs);

    // Für Sonderfälle (listPeers): raw access (optional, wenn du willst)
    std::optional<std::vector<uint8_t>> readOneFrame(int timeoutMs);

    std::optional<std::vector<uint8_t>> requestResponseAny(
        const std::vector<uint8_t> &cmdPayload,
        const std::vector<uint8_t> &wantedCodes,
        int timeoutMs);

private:
    std::atomic<bool> m_running;

    std::thread m_rxThread;

    // callbacks
    std::mutex m_cbMutex;
    PushCallback m_pushCb;

    // single outstanding request
    std::mutex m_reqMutex;
    std::condition_variable m_reqCv;
    bool m_waiting;
    std::vector<uint8_t> m_wantedCodes;
    std::vector<uint8_t> m_lastResp;

    std::unique_ptr<IByteStream> m_stream;
    std::optional<MeshCoreFramer> m_framer;

    void rxLoop();
};