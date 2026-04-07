#pragma once

#include "MeshCoreClient.h"
#include "MeshDB.h"
#include "RoomAuthManager.h"

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <array>

class AppRuntime
{
public:
    explicit AppRuntime(MeshCoreClient& client);

    bool InitializeClient();
    void StartupSync();
    void Tick();

    void RequestContactSync();
    void NotifyRoomLoginSuccess(const std::array<uint8_t, 6>& prefix6);
    void NotifyRoomLoginFail();
    void SetRoomPassword(uint32_t roomNodeId, const std::string& password);
    void CheckAndApplyCompanionConfig(bool forceApply);
    bool ApplyCompanionConfig(const MeshDB::CompanionConfig& cfg);
    void ProcessDiscoverQueue();
    bool ProcessSingleDiscoverJob(const MeshDB::DiscoverJob& job);

    bool CreateOrUpdateChannel(
        uint8_t channelIdx,
        const std::string& name,
        const std::array<uint8_t, 16>& secret,
        uint8_t joinMode,
        bool isDefault);

    bool RemoveChannel(uint8_t channelIdx);

    std::optional<uint8_t> FindNextFreeChannelIdx();

    bool CreatePublicChannel(const std::string& name);
    bool CreatePrivateChannel(const std::string& name, const std::string& secretHex);

private:
    bool ShouldRunContactSync();
    void SyncContacts();
    void SyncChannels();

    void ProcessOutgoingQueue();
    bool ProcessSingleOutgoingTx(const MeshDB::OutgoingTx& tx);

    bool ProcessDirectTx(const MeshDB::OutgoingTx& tx);
    bool ProcessRoomTx(const MeshDB::OutgoingTx& tx);

    std::optional<uint32_t> ResolveRoomNodeId(const MeshDB::OutgoingTx& tx);
    bool EnsureRoomReady(const MeshDB::OutgoingTx& tx, uint32_t roomNodeId);
    bool StartRoomLogin(const MeshDB::OutgoingTx& tx, uint32_t roomNodeId);

    uint32_t DetermineSenderTimestamp(const MeshDB::OutgoingTx& tx) const;
    bool ProcessFloodAdvertTx(const MeshDB::OutgoingTx& tx);

    void MarkWaitingForAck(
        const MeshDB::OutgoingTx& tx,
        const MeshCoreClient::TxQueued& queued,
        uint32_t senderTimestamp);

    void HandleSendFailure(const MeshDB::OutgoingTx& tx, const char* reason);

    void ProcessAckTimeouts();
    void HandleAckTimeout(const MeshDB::OutgoingTx& tx);
    void RequestRoomPassword(const MeshDB::OutgoingTx& tx, uint32_t roomNodeId);
    void HandleRoomLoginFailure(const MeshDB::OutgoingTx& tx,
                                            uint32_t roomNodeId,
                                            const char* reason,
                                            uint32_t retryDelaySec);
    bool ProcessChannelTx(const MeshDB::OutgoingTx& tx);

    static bool ParseHex16(const std::string& hex, std::array<uint8_t, 16>& out);
    static std::array<uint8_t, 16> DerivePublicChannelSecret(const std::string& name);  

    MeshCoreClient& m_client;

    std::mutex m_syncMutex;
    bool m_syncContactsRequested;
    std::chrono::steady_clock::time_point m_syncContactsAt;

    RoomAuthManager m_roomAuth;

    void ProcessPendingChannelSync();
    bool ApplyPendingChannelUpsert(const MeshDB::ChannelRecord& rec);
    bool ApplyPendingChannelDelete(const MeshDB::ChannelRecord& rec);
    std::chrono::steady_clock::time_point m_nextChannelSyncPollAt;
};