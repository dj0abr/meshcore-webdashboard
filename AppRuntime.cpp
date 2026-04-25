#include "AppRuntime.h"
#include "DataConnector.h"

#include <ctime>
#include <iostream>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <openssl/sha.h>

static constexpr unsigned DISCOVER_COOLDOWN_SECONDS = 5;
static constexpr unsigned RADIO_STATUS_POLL_SECONDS = 10;

static int HexNibble(char c)
{
    if (c >= '0' && c <= '9')
    {
        return c - '0';
    }

    if (c >= 'a' && c <= 'f')
    {
        return 10 + (c - 'a');
    }

    if (c >= 'A' && c <= 'F')
    {
        return 10 + (c - 'A');
    }

    return -1;
}

static bool ParseHex16Local(const std::string& hex, std::array<uint8_t, 16>& out)
{
    if (hex.size() != 32)
    {
        return false;
    }

    for (size_t i = 0; i < 16; i++)
    {
        const int hi = HexNibble(hex[i * 2]);
        const int lo = HexNibble(hex[i * 2 + 1]);

        if (hi < 0 || lo < 0)
        {
            return false;
        }

        out[i] = static_cast<uint8_t>((hi << 4) | lo);
    }

    return true;
}

static bool ParseHex32Local(const std::string& hex, std::array<uint8_t, 32>& out)
{
    if (hex.size() != 64)
    {
        return false;
    }

    for (size_t i = 0; i < 32; i++)
    {
        const int hi = HexNibble(hex[i * 2]);
        const int lo = HexNibble(hex[i * 2 + 1]);

        if (hi < 0 || lo < 0)
        {
            return false;
        }

        out[i] = static_cast<uint8_t>((hi << 4) | lo);
    }

    return true;
}

static std::string Hex8(const std::array<uint8_t, 8>& data)
{
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    out.reserve(16);

    for (uint8_t b : data)
    {
        out.push_back(hex[(b >> 4) & 0x0F]);
        out.push_back(hex[b & 0x0F]);
    }

    return out;
}

static std::string Hex16(const std::array<uint8_t, 16>& data)
{
    static const char* hex = "0123456789ABCDEF";
    std::string out;
    out.reserve(32);

    for (uint8_t b : data)
    {
        out.push_back(hex[(b >> 4) & 0x0F]);
        out.push_back(hex[b & 0x0F]);
    }

    return out;
}

static std::string TrimCopy(const std::string& s)
{
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
    {
        start++;
    }

    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
    {
        end--;
    }

    return s.substr(start, end - start);
}

static bool IsAllZero16(const std::array<uint8_t, 16>& data)
{
    for (uint8_t b : data)
    {
        if (b != 0)
        {
            return false;
        }
    }

    return true;
}

AppRuntime::AppRuntime(MeshCoreClient& client)
    : m_client(client)
    , m_syncContactsRequested(false)
    , m_syncContactsAt()
    , m_nextChannelSyncPollAt(std::chrono::steady_clock::now())
    , m_nextRadioStatusPollAt(std::chrono::steady_clock::now()) 
{
}

bool AppRuntime::InitializeClient()
{
    if (!m_client.isConnected())
    {
        std::cerr << "not connected\n";
        return false;
    }

    if (!m_client.setRadioParams(869618000, 62500, 8, 8, false))
    {
        std::cerr << "setRadioParams() failed\n";
        return false;
    }

    m_client.setManualAddContacts(false);
    if (!m_client.syncClock())
    {
        std::cerr << "syncClock() failed\n";
        return false;
    }
    m_client.sendSelfAdvert(true);

    auto id = m_client.getNodeID();
    auto t = m_client.getTime();

    if (id.has_value() && t.has_value())
    {
        std::cout << DataConnector::u32hex(*id) << "|* clock\n";
        std::cout << "Current time : " << DataConnector::formatLocalTime(*t)
                  << " (" << *t << ")\n\n";
    }

    return true;
}

void AppRuntime::StartupSync()
{
    MeshDB::ClearNodesTable();
    MeshDB::ClearChannelsTable();
    MeshDB::ClearTxBoxTable();
    SyncContacts();
    SyncChannels();
}

void AppRuntime::Tick()
{
    if (ShouldRunContactSync())
    {
        SyncContacts();
    }

    if (std::chrono::steady_clock::now() >= m_nextChannelSyncPollAt)
    {
        ProcessPendingChannelSync();
        m_nextChannelSyncPollAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(500);
    }
    
    PollRadioStatus();
    ProcessCompanionActions();
    ProcessOutgoingQueue();
    ProcessDiscoverQueue();
    ProcessAckTimeouts();
}

void AppRuntime::RequestContactSync()
{
    std::lock_guard<std::mutex> lock(m_syncMutex);

    m_syncContactsRequested = true;
    m_syncContactsAt = std::chrono::steady_clock::now() + std::chrono::milliseconds(300);
}

bool AppRuntime::ShouldRunContactSync()
{
    std::lock_guard<std::mutex> lock(m_syncMutex);

    if (!m_syncContactsRequested)
    {
        return false;
    }

    if (std::chrono::steady_clock::now() < m_syncContactsAt)
    {
        return false;
    }

    m_syncContactsRequested = false;
    return true;
}

void AppRuntime::SyncContacts()
{
    auto peers = m_client.listPeers();

    if (!peers.has_value())
    {
        std::cout << "[SYNC] listPeers() failed\n";
        return;
    }

    std::cout << "[SYNC] peers=" << peers->size() << "\n";

    for (const auto& p : *peers)
    {
        DataConnector::AdvertInfo adv {};

        adv.nodeId = p.nodeId();
        adv.type = static_cast<DataConnector::AdvertType>(p.type);
        adv.flags = p.flags;
        adv.lastAdvert = p.lastAdvert;
        adv.name = p.name;
        adv.publicKey = p.publicKey;
        adv.advLatE6 = p.advLatE6;
        adv.advLonE6 = p.advLonE6;

        DataConnector::Emit(adv);
    }
}

void AppRuntime::SyncChannels()
{
    const uint8_t maxChannels = m_client.maxChannels();

    std::cout << "[SYNC] channels max=" << unsigned(maxChannels) << "\n";

    for (uint8_t idx = 0; idx < maxChannels; idx++)
    {
        auto info = m_client.getChannelInfo(idx);

        if (!info.has_value())
        {
            std::cout << "[SYNC] getChannelInfo(" << unsigned(idx) << ") failed\n";
            continue;
        }

        if (info->isEmpty())
        {
            continue;
        }

        const bool isDefault = (idx == 0);
        const bool enabled = true;

        uint8_t joinMode = 0;

        if (!info->name.empty() && (info->name[0] == '#'))
        {
            joinMode = 1;
        }

        const std::string keyHex = Hex16(info->secret);

        if (!MeshDB::UpsertLocalChannel(
            info->channelIdx,
            info->name,
            joinMode,
            "",
            keyHex,
            enabled,
            isDefault))
        {
            std::cout << "[SYNC] UpsertChannel(" << unsigned(idx) << ") failed\n";
            continue;
        }

        std::cout
            << "[SYNC] channel "
            << unsigned(info->channelIdx)
            << " name=\"" << info->name
            << "\" key=" << keyHex
            << "\n";
    }
}

void AppRuntime::ProcessOutgoingQueue()
{
    while (true)
    {
        auto tx = MeshDB::FetchNextQueuedTx();

        if (!tx.has_value())
        {
            break;
        }

        if (!ProcessSingleOutgoingTx(*tx))
        {
            break;
        }
    }
}

bool AppRuntime::ProcessFloodAdvertTx(const MeshDB::OutgoingTx& tx)
{
    std::cout << "ProcessFloodAdvertTx called for tx id=" << tx.id << std::endl;

    if (!m_client.sendSelfAdvert(true))
    {
        HandleSendFailure(tx, "flood advert failed");
        return false;
    }

    if (!MeshDB::MarkTxDone(tx.id))
    {
        HandleSendFailure(tx, "failed to mark advert tx done");
        return false;
    }

    return true;
}

bool AppRuntime::ProcessSingleOutgoingTx(const MeshDB::OutgoingTx& tx)
{
    switch (tx.kind)
    {
        case MeshDB::OutgoingTx::Kind::Direct:
            return ProcessDirectTx(tx);

        case MeshDB::OutgoingTx::Kind::Room:
            return ProcessRoomTx(tx);

        case MeshDB::OutgoingTx::Kind::FloodAdvert:
            return ProcessFloodAdvertTx(tx);

        case MeshDB::OutgoingTx::Kind::Channel:
            return ProcessChannelTx(tx);

        default:
            HandleSendFailure(tx, "unknown tx kind");
            return false;
    }
}

bool AppRuntime::ProcessChannelTx(const MeshDB::OutgoingTx& tx)
{
    const auto channelOpt = MeshDB::FindChannelByKeyHex(tx.channelKeyHex);

    if (!channelOpt.has_value())
    {
        HandleSendFailure(tx, "channel not found");
        return false;
    }

    if (!channelOpt->enabled)
    {
        HandleSendFailure(tx, "channel disabled");
        return false;
    }

    const uint8_t runtimeChannelIdx = channelOpt->channelIdx;
    const uint32_t senderTimestamp = DetermineSenderTimestamp(tx);

    const auto result = m_client.sendChannelMessageEx(
        runtimeChannelIdx,
        tx.messageText,
        tx.retryCount,
        senderTimestamp);

    if (!result.has_value())
    {
        HandleSendFailure(tx, "channel send failed");
        return false;
    }
    if (!MeshDB::MarkTxDone(tx.id))
    {
        HandleSendFailure(tx, "failed to mark channel tx done");
        return false;
    }

    return true;
}

std::optional<uint32_t> AppRuntime::ResolveRoomNodeId(const MeshDB::OutgoingTx& tx)
{
    if (tx.roomNodeId != 0)
    {
        return tx.roomNodeId;
    }

    if (tx.roomName.empty())
    {
        return std::nullopt;
    }

    const auto peers = m_client.listPeers();

    if (peers.has_value())
    {
        for (const auto& peer : *peers)
        {
            if (peer.type != 3)
            {
                continue;
            }

            if (peer.name == tx.roomName)
            {
                return peer.nodeId();
            }
        }
    }

    return MeshDB::FindRoomNodeIdByName(tx.roomName);
}

void AppRuntime::HandleRoomLoginFailure(
    const MeshDB::OutgoingTx& tx,
    uint32_t roomNodeId,
    const char* reason,
    uint32_t retryDelaySec)
{
    m_roomAuth.Reset(roomNodeId);

    const uint8_t nextRetryCount = static_cast<uint8_t>(tx.retryCount + 1);

    std::cout << "[ROOM] login retry scheduled"
              << " roomNodeId=" << roomNodeId
              << " txId=" << tx.id
              << " txRetryCount=" << static_cast<unsigned>(tx.retryCount)
              << " nextRetryCount=" << static_cast<unsigned>(nextRetryCount)
              << " maxRetries=" << static_cast<unsigned>(tx.maxRetries)
              << " delaySec=" << retryDelaySec
              << " reason='" << reason << "'"
              << "\n";

    if (nextRetryCount > tx.maxRetries)
    {
        std::cout << "[ROOM] login retries exhausted"
                  << " roomNodeId=" << roomNodeId
                  << " txId=" << tx.id
                  << " finalRetryCount=" << static_cast<unsigned>(tx.retryCount)
                  << " maxRetries=" << static_cast<unsigned>(tx.maxRetries)
                  << "\n";

        MeshDB::MarkTxFailed(tx.id, reason);
        return;
    }

    const uint32_t nextTry =
        static_cast<uint32_t>(std::time(nullptr)) + retryDelaySec;

    MeshDB::RequeueTx(
        tx.id,
        nextRetryCount,
        reason,
        nextTry);
}

bool AppRuntime::EnsureRoomReady(const MeshDB::OutgoingTx& tx, uint32_t roomNodeId)
{
    const auto state = m_roomAuth.GetState(roomNodeId);

    switch (state)
    {
        case RoomAuthManager::State::LoggedIn:
        {
            return true;
        }

        case RoomAuthManager::State::Unknown:
        {
            if (!m_roomAuth.HasPassword(roomNodeId))
            {
                const auto dbPassword = MeshDB::FindRoomPassword(roomNodeId);

                if (dbPassword.has_value())
                {
                    m_roomAuth.SetPassword(roomNodeId, *dbPassword);
                }
            }

            if (!m_roomAuth.HasPassword(roomNodeId))
            {
                if (m_roomAuth.ShouldRequestPassword(roomNodeId))
                {
                    m_roomAuth.MarkPasswordRequested(roomNodeId);
                    RequestRoomPassword(tx, roomNodeId);
                }

                MeshDB::MarkTxDeferred(
                    tx.id,
                    "room password required",
                    static_cast<uint32_t>(std::time(nullptr)) + 5U);
                return false;
            }

            if (!m_roomAuth.BeginLogin(
                    roomNodeId,
                    tx.id,
                    static_cast<uint32_t>(std::time(nullptr)),
                    15U))
            {
                MeshDB::MarkTxDeferred(
                    tx.id,
                    "another room login pending",
                    static_cast<uint32_t>(std::time(nullptr)) + 5U);
                return false;
            }

            return StartRoomLogin(tx, roomNodeId);
        }

        case RoomAuthManager::State::LoginPending:
        {
            printf("EnsureRoomReady: room login pending\n");

            const uint32_t nowSec = static_cast<uint32_t>(std::time(nullptr));

            if (m_roomAuth.IsLoginTimedOut(roomNodeId, nowSec))
            {
                printf("EnsureRoomReady: room login timed out\n");
                HandleRoomLoginFailure(tx, roomNodeId, "room login timeout", 5U);
                return false;
            }

            MeshDB::MarkTxDeferred(
                tx.id,
                "room login pending",
                nowSec + 1U);

            return false;
        }

        case RoomAuthManager::State::LoginFailed:
        {
            printf("EnsureRoomReady: room login failed\n");
            HandleRoomLoginFailure(tx, roomNodeId, "room login failed", 15U);
            return false;
        }
    }

    return false;
}

bool AppRuntime::ProcessRoomTx(const MeshDB::OutgoingTx& tx)
{
    const auto roomNodeIdOpt = ResolveRoomNodeId(tx);

    if (!roomNodeIdOpt.has_value())
    {
        HandleSendFailure(tx, "room not found");
        return false;
    }

    const uint32_t roomNodeId = *roomNodeIdOpt;

    if (tx.roomNodeId == 0)
    {
        MeshDB::UpdateTxRoomNodeId(tx.id, roomNodeId);
    }

    if (!EnsureRoomReady(tx, roomNodeId))
    {
        return false;
    }

    const uint32_t senderTimestamp = DetermineSenderTimestamp(tx);

    std::cout << "[ROOM] sendRoomMessageEx()"
            << " roomNodeId=" << roomNodeId
            << " txId=" << tx.id
            << " attempt=" << static_cast<unsigned>(tx.retryCount)
            << " maxRetries=" << static_cast<unsigned>(tx.maxRetries)
            << " senderTimestamp=" << senderTimestamp
            << " textLen=" << tx.messageText.size()
            << "\n";

    const auto result = m_client.sendRoomMessageEx(
        roomNodeId,
        tx.messageText,
        tx.retryCount,
        senderTimestamp);

    if (!result.has_value())
    {
        m_roomAuth.Reset(roomNodeId);
        HandleSendFailure(
            tx,
            "room send failed (room offline? login expired? link busy?)");

        return false;
    }

    MarkWaitingForAck(tx, *result, senderTimestamp);
    return true;
}

bool AppRuntime::ProcessDirectTx(const MeshDB::OutgoingTx& tx)
{
    const uint32_t senderTimestamp = DetermineSenderTimestamp(tx);

    std::optional<MeshCoreClient::TxQueued> result;

    if (tx.targetNodeId != 0)
    {
        result = m_client.sendMessageEx(
            tx.targetNodeId,
            tx.messageText,
            tx.retryCount,
            senderTimestamp);
    }
    else
    {
        result = m_client.sendMessageToNameEx(
            tx.targetName,
            tx.messageText,
            tx.retryCount,
            senderTimestamp);
    }

    if (!result.has_value())
    {
        HandleSendFailure(
            tx,
            "send failed (unknown contact? ambiguous name? link busy?)");
        return false;
    }

    MarkWaitingForAck(tx, *result, senderTimestamp);
    return true;
}

uint32_t AppRuntime::DetermineSenderTimestamp(const MeshDB::OutgoingTx& tx) const
{
    if (tx.senderTimestamp != 0)
    {
        return tx.senderTimestamp;
    }

    return static_cast<uint32_t>(std::time(nullptr));
}

void AppRuntime::MarkWaitingForAck(
    const MeshDB::OutgoingTx& tx,
    const MeshCoreClient::TxQueued& queued,
    uint32_t senderTimestamp)
{
    MeshDB::MarkTxWaitingAck(
        tx.id,
        queued.nodeId,
        senderTimestamp,
        tx.retryCount,
        queued.ack,
        queued.suggestedTimeoutMs);
}

void AppRuntime::HandleSendFailure(const MeshDB::OutgoingTx& tx, const char* reason)
{
    const uint8_t nextRetryCount = static_cast<uint8_t>(tx.retryCount + 1);

    if (nextRetryCount > tx.maxRetries)
    {
        MeshDB::MarkTxFailed(tx.id, reason);
        return;
    }

    const uint32_t nextTry = static_cast<uint32_t>(std::time(nullptr)) + 1U;

    MeshDB::RequeueTx(
        tx.id,
        nextRetryCount,
        reason,
        nextTry);
}

void AppRuntime::ProcessAckTimeouts()
{
    const auto expired = MeshDB::FetchTimedOutWaitingTx(8);

    for (const auto& tx : expired)
    {
        HandleAckTimeout(tx);
    }
}

void AppRuntime::HandleAckTimeout(const MeshDB::OutgoingTx& tx)
{
    if (tx.retryCount > tx.maxRetries)
    {
        MeshDB::MarkTxFailed(
            tx.id,
            "no confirm after retries");
        return;
    }

    const uint32_t nextTry = static_cast<uint32_t>(std::time(nullptr)) + 1U;

    MeshDB::RequeueTx(
        tx.id,
        static_cast<uint8_t>(tx.retryCount + 1),
        "ack timeout",
        nextTry);
}

void AppRuntime::NotifyRoomLoginSuccess(const std::array<uint8_t, 6>& prefix6)
{
    const auto resolvedOpt = m_roomAuth.ResolvePendingLoginSuccess();

    if (!resolvedOpt.has_value())
    {
        std::cout << "[ROOM] LOGIN_SUCCESS without pending room"
                  << " prefix="
                  << DataConnector::hexBytes(prefix6.data(), prefix6.size())
                  << "\n";
        return;
    }

    MeshDB::ResetTxRetryCount(resolvedOpt->txId);

    std::cout << "[ROOM] login success for room node "
              << resolvedOpt->roomNodeId
              << " prefix="
              << DataConnector::hexBytes(prefix6.data(), prefix6.size())
              << " txId=" << resolvedOpt->txId
              << " txRetryCountReset=1"
              << "\n";
}

void AppRuntime::NotifyRoomLoginFail()
{
    const auto resolvedOpt = m_roomAuth.ResolvePendingLoginFail();

    if (!resolvedOpt.has_value())
    {
        std::cout << "[ROOM] LOGIN_FAIL without pending room\n";
        return;
    }

    std::cout << "[ROOM] login failed for room node "
              << resolvedOpt->roomNodeId
              << " txId=" << resolvedOpt->txId
              << "\n";
}

void AppRuntime::SetRoomPassword(uint32_t roomNodeId, const std::string& password)
{
    m_roomAuth.SetPassword(roomNodeId, password);
    m_roomAuth.ClearPasswordRequested(roomNodeId);
    m_roomAuth.Reset(roomNodeId);
}

void AppRuntime::RequestRoomPassword(const MeshDB::OutgoingTx& tx, uint32_t roomNodeId)
{
    DataConnector::RoomPasswordRequiredInfo info {};
    info.roomNodeId = roomNodeId;
    info.roomName = !tx.roomName.empty() ? tx.roomName : std::string("<unknown-room>");

    DataConnector::Emit(info);
}

bool AppRuntime::StartRoomLogin(const MeshDB::OutgoingTx& tx, uint32_t roomNodeId)
{
    std::cout << "[ROOM] StartRoomLogin roomNodeId="
              << roomNodeId
              << " roomName='"
              << tx.roomName
              << "'"
              << "\n";

    const auto passwordOpt = m_roomAuth.GetPassword(roomNodeId);

    if (!passwordOpt.has_value())
    {
        MeshDB::MarkTxDeferred(
            tx.id,
            "room password missing",
            static_cast<uint32_t>(std::time(nullptr)) + 5U);
        return false;
    }

    std::cout << "[ROOM] calling loginToRoomEx()"
              << " roomNodeId=" << roomNodeId
              << " passwordLen=" << passwordOpt->size()
              << "\n";

    const auto result = m_client.loginToRoomEx(roomNodeId, *passwordOpt);

    if (!result.has_value())
    {
        std::cout << "[ROOM] loginToRoomEx() failed\n";
    }
    else
    {
        std::cout << "[ROOM] loginToRoomEx() queued ack="
                  << *result
                  << "\n";

        m_roomAuth.RefreshLoginDeadline(
            roomNodeId,
            static_cast<uint32_t>(std::time(nullptr)),
            15U);
    }

    if (!result.has_value())
    {
        HandleRoomLoginFailure(tx, roomNodeId, "room login command failed", 5U);
        return false;
    }

    MeshDB::MarkTxDeferred(
        tx.id,
        "room login pending",
        static_cast<uint32_t>(std::time(nullptr)) + 5U);

    return false;
}

bool AppRuntime::ApplyCompanionConfig(const MeshDB::CompanionConfig& cfg)
{
    if (!m_client.isConnected())
    {
        MeshDB::MarkCompanionConfigApplyFailed("companion not connected");
        return false;
    }

    if (!m_client.setAdvertName(cfg.name))
    {
        MeshDB::MarkCompanionConfigApplyFailed("setAdvertName failed");
        return false;
    }

    if (!m_client.setAdvertLocation(cfg.latitudeE6, cfg.longitudeE6))
    {
        MeshDB::MarkCompanionConfigApplyFailed("setAdvertLocation failed");
        return false;
    }

    if (!m_client.setRadioParams(
            869618000,
            62500,
            8,
            8,
            false))
    {
        MeshDB::MarkCompanionConfigApplyFailed("setRadioParams failed");
        return false;
    }

    if (!m_client.setRadioTxPower(22))
    {
        MeshDB::MarkCompanionConfigApplyFailed("setRadioTxPower failed");
        return false;
    }

    if (!m_client.sendSelfAdvert(true))
    {
        MeshDB::MarkCompanionConfigApplyFailed("sendSelfAdvert failed");
        return false;
    }

    MeshDB::MarkCompanionConfigApplied();
    return true;
}

void AppRuntime::CheckAndApplyCompanionConfig(bool forceApply)
{
    const auto cfgOpt = MeshDB::LoadCompanionConfig();

    if (!cfgOpt.has_value())
    {
        return;
    }

    const auto& cfg = *cfgOpt;

    if (!forceApply && !cfg.applyPending)
    {
        return;
    }

    ApplyCompanionConfig(cfg);
}

void AppRuntime::ProcessDiscoverQueue()
{
    if (MeshDB::HasRecentDiscoverJob(DISCOVER_COOLDOWN_SECONDS))
    {
        return;
    }

    auto jobOpt = MeshDB::FetchLatestQueuedDiscoverJob();

    if (!jobOpt.has_value())
    {
        return;
    }

    const MeshDB::DiscoverJob& job = *jobOpt;

    if (!MeshDB::SkipOlderQueuedDiscoverJobs(job.id))
    {
        std::cerr
            << "[discover] failed to skip older queued jobs before starting job "
            << job.id
            << "\n";
    }

    std::cout
        << "[discover] latest queued job found: id="
        << job.id
        << " type_filter=" << unsigned(job.typeFilter)
        << " requested_by=" << job.requestedBy
        << "\n";

    if (!ProcessSingleDiscoverJob(job))
    {
        std::cout
            << "[discover] job "
            << job.id
            << " finished with failure\n";
    }

    MeshDB::CleanupOldDiscoverJobs(20);
}

bool AppRuntime::ProcessSingleDiscoverJob(const MeshDB::DiscoverJob& job)
{
    if (!m_client.isConnected())
    {
        MeshDB::MarkDiscoverJobFailed(job.id, "client not connected");
        return false;
    }

    const uint32_t requestTag = static_cast<uint32_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());

    if (!MeshDB::MarkDiscoverJobRunning(job.id, requestTag))
    {
        std::cerr
            << "[discover] failed to mark job "
            << job.id
            << " as running\n";
        return false;
    }

    std::cout
        << "[discover] starting job id="
        << job.id
        << " filter=0x" << std::hex << unsigned(job.typeFilter)
        << std::dec
        << " request_tag=" << requestTag
        << "\n";

    std::optional<std::vector<MeshCoreClient::DiscoverResult>> results;

    switch (job.typeFilter)
    {
        case MeshCoreProto::DISCOVER_FILTER_REPEATER:
        {
            results = m_client.discoverRepeaters(10000, 8000, 20000);
            break;
        }

        default:
        {
            std::ostringstream oss;
            oss
                << "unsupported discover filter: "
                << unsigned(job.typeFilter);

            MeshDB::MarkDiscoverJobFailed(job.id, oss.str());
            return false;
        }
    }

    if (!results.has_value())
    {
        MeshDB::MarkDiscoverJobFailed(job.id, "discover request failed");
        return false;
    }

    for (const auto& r : *results)
    {
        MeshDB::DiscoverResultRow row {};
        row.lastJobId = job.id;
        row.nodeIdHex = Hex8(r.nodeId);
        row.snrRxDb = r.snrRxDb;
        row.snrTxDb = r.snrTxDb;
        row.rssiDbm = r.rssiDbm;
        row.sourceCode = r.sourceCode;

        if (!MeshDB::InsertDiscoverResult(row))
        {
            std::ostringstream oss;
            oss
                << "failed to insert discover result for node "
                << row.nodeIdHex;

            MeshDB::MarkDiscoverJobFailed(job.id, oss.str());
            return false;
        }

        std::cout
            << "[discover] stored result job_id="
            << job.id
            << " node=" << row.nodeIdHex
            << " snr_rx=" << row.snrRxDb
            << " snr_tx=" << row.snrTxDb
            << " rssi=" << row.rssiDbm
            << "\n";
    }

    if (!MeshDB::UpdateDiscoverJobResultCount(
            job.id,
            static_cast<uint32_t>(results->size())))
    {
        std::cerr
            << "[discover] failed to update result_count for job "
            << job.id
            << "\n";
    }

    if (!MeshDB::MarkDiscoverJobDone(job.id))
    {
        std::cerr
            << "[discover] failed to mark job "
            << job.id
            << " as done\n";
        return false;
    }

    std::cout
        << "[discover] job "
        << job.id
        << " done, "
        << results->size()
        << " result(s)\n";

    return true;
}

bool AppRuntime::CreateOrUpdateChannel(
    uint8_t channelIdx,
    const std::string& name,
    const std::array<uint8_t, 16>& secret,
    uint8_t joinMode,
    bool isDefault)
{
    const std::string cleanName = TrimCopy(name);

    if (channelIdx >= m_client.maxChannels())
    {
        std::cout << "[CHANNEL] invalid idx " << unsigned(channelIdx) << "\n";
        return false;
    }

    if (cleanName.empty())
    {
        std::cout << "[CHANNEL] name is empty\n";
        return false;
    }

    if (IsAllZero16(secret))
    {
        std::cout << "[CHANNEL] secret is all zero\n";
        return false;
    }

    if (!m_client.setChannel(channelIdx, cleanName, secret))
    {
        std::cout << "[CHANNEL] setChannel failed for idx " << unsigned(channelIdx) << "\n";
        return false;
    }

    auto info = m_client.getChannelInfo(channelIdx);
    if (!info.has_value())
    {
        std::cout << "[CHANNEL] verification getChannelInfo failed for idx " << unsigned(channelIdx) << "\n";
        return false;
    }

    if (info->isEmpty())
    {
        std::cout << "[CHANNEL] verification returned empty slot for idx " << unsigned(channelIdx) << "\n";
        return false;
    }

    const bool enabled = true;
    const std::string keyHex = Hex16(info->secret);

    if (!MeshDB::UpsertLocalChannel(
            info->channelIdx,
            info->name,
            joinMode,
            "",
            keyHex,
            enabled,
            isDefault))
    {
        std::cout << "[CHANNEL] UpsertChannel failed for idx " << unsigned(channelIdx) << "\n";
        return false;
    }

    std::cout
        << "[CHANNEL] saved idx=" << unsigned(info->channelIdx)
        << " name=\"" << info->name
        << "\" key=" << keyHex
        << "\n";

    return true;
}

bool AppRuntime::RemoveChannel(uint8_t channelIdx)
{
    if (channelIdx >= m_client.maxChannels())
    {
        std::cout << "[CHANNEL] invalid idx " << unsigned(channelIdx) << "\n";
        return false;
    }

    if (channelIdx == 0)
    {
        std::cout << "[CHANNEL] refusing to remove default channel 0\n";
        return false;
    }

    const auto recOpt = MeshDB::FindChannelByIdx(channelIdx);

    if (!recOpt.has_value())
    {
        std::cout << "[CHANNEL] db channel not found for idx " << unsigned(channelIdx) << "\n";
        return false;
    }

    if (!m_client.deleteChannel(channelIdx))
    {
        std::cout << "[CHANNEL] deleteChannel failed for idx " << unsigned(channelIdx) << "\n";
        return false;
    }

    auto info = m_client.getChannelInfo(channelIdx);
    if (!info.has_value())
    {
        std::cout << "[CHANNEL] verification getChannelInfo failed for idx " << unsigned(channelIdx) << "\n";
        return false;
    }

    if (!info->isEmpty())
    {
        std::cout << "[CHANNEL] slot still not empty after delete for idx " << unsigned(channelIdx) << "\n";
        return false;
    }

    if (!MeshDB::DeleteChannelByKeyHex(recOpt->keyHex))
    {
        std::cout << "[CHANNEL] DeleteChannelByKeyHex DB failed for idx " << unsigned(channelIdx) << "\n";
        return false;
    }

    std::cout
        << "[CHANNEL] removed idx=" << unsigned(channelIdx)
        << " key=" << recOpt->keyHex
        << "\n";

    return true;
}

bool AppRuntime::ParseHex16(const std::string& hex, std::array<uint8_t, 16>& out)
{
    if (hex.size() != 32)
    {
        return false;
    }

    for (size_t i = 0; i < 16; i++)
    {
        const int hi = HexNibble(hex[i * 2]);
        const int lo = HexNibble(hex[i * 2 + 1]);

        if (hi < 0 || lo < 0)
        {
            return false;
        }

        out[i] = static_cast<uint8_t>((hi << 4) | lo);
    }

    return true;
}

std::array<uint8_t, 16> AppRuntime::DerivePublicChannelSecret(const std::string& name)
{
    std::array<uint8_t, 16> out {};
    unsigned char digest[SHA256_DIGEST_LENGTH] = {0};

    SHA256(
        reinterpret_cast<const unsigned char*>(name.data()),
        name.size(),
        digest);

    for (size_t i = 0; i < 16; i++)
    {
        out[i] = digest[i];
    }

    return out;
}

std::optional<uint8_t> AppRuntime::FindNextFreeChannelIdx()
{
    const uint8_t maxChannels = m_client.maxChannels();

    for (uint8_t idx = 1; idx < maxChannels; idx++)
    {
        auto info = m_client.getChannelInfo(idx);
        if (!info.has_value())
        {
            continue;
        }

        if (info->isEmpty())
        {
            return idx;
        }
    }

    return std::nullopt;
}

bool AppRuntime::CreatePublicChannel(const std::string& name)
{
    const std::string cleanName = TrimCopy(name);

    if (cleanName.empty())
    {
        std::cout << "[CHANNEL] public name is empty\n";
        return false;
    }

    auto freeIdx = FindNextFreeChannelIdx();
    if (!freeIdx.has_value())
    {
        std::cout << "[CHANNEL] no free channel slot\n";
        return false;
    }

    const std::array<uint8_t, 16> secret = DerivePublicChannelSecret(cleanName);

    const uint8_t joinMode = (!cleanName.empty() && cleanName[0] == '#') ? 1 : 0;
    const bool isDefault = false;

    return CreateOrUpdateChannel(*freeIdx, cleanName, secret, joinMode, isDefault);
}

bool AppRuntime::CreatePrivateChannel(const std::string& name, const std::string& secretHex)
{
    const std::string cleanName = TrimCopy(name);

    if (cleanName.empty())
    {
        std::cout << "[CHANNEL] private name is empty\n";
        return false;
    }

    std::array<uint8_t, 16> secret {};
    if (!ParseHex16(secretHex, secret))
    {
        std::cout << "[CHANNEL] invalid private secret hex\n";
        return false;
    }

    if (IsAllZero16(secret))
    {
        std::cout << "[CHANNEL] private secret must not be all zero\n";
        return false;
    }

    auto freeIdx = FindNextFreeChannelIdx();
    if (!freeIdx.has_value())
    {
        std::cout << "[CHANNEL] no free channel slot\n";
        return false;
    }

    const uint8_t joinMode = 2;
    const bool isDefault = false;

    return CreateOrUpdateChannel(*freeIdx, cleanName, secret, joinMode, isDefault);
}

void AppRuntime::ProcessPendingChannelSync()
{
    const auto pending = MeshDB::ListPendingChannelSync();

    for (const auto& rec : pending)
    {
        bool ok = false;

        if (rec.syncAction == "delete")
        {
            ok = ApplyPendingChannelDelete(rec);
        }
        else
        {
            ok = ApplyPendingChannelUpsert(rec);
        }

        if (!ok)
        {
            // Fehler wurde in der jeweiligen Funktion schon markiert
        }
    }
}

bool AppRuntime::ApplyPendingChannelUpsert(const MeshDB::ChannelRecord& rec)
{
    std::cout
        << "[CHANNEL] pending upsert idx=" << unsigned(rec.channelIdx)
        << " name=\"" << rec.name
        << "\" key=" << rec.keyHex
        << "\n";

    if (rec.channelIdx >= m_client.maxChannels())
    {
        std::cout << "[CHANNEL] upsert failed: channel idx out of range\n";
        MeshDB::MarkChannelSyncErrorByKeyHex(rec.keyHex, "channel idx out of range");
        return false;
    }

    std::array<uint8_t, 16> secret {};

    if (!ParseHex16Local(rec.keyHex, secret))
    {
        std::cout << "[CHANNEL] upsert failed: invalid key_hex \"" << rec.keyHex << "\"\n";
        MeshDB::MarkChannelSyncErrorByKeyHex(rec.keyHex, "invalid key_hex");
        return false;
    }

    if (!m_client.setChannel(rec.channelIdx, rec.name, secret))
    {
        std::cout
            << "[CHANNEL] upsert failed: setChannel failed for idx="
            << unsigned(rec.channelIdx)
            << " name=\"" << rec.name << "\"\n";

        MeshDB::MarkChannelSyncErrorByKeyHex(rec.keyHex, "setChannel failed");
        return false;
    }

    auto info = m_client.getChannelInfo(rec.channelIdx);
    if (!info.has_value())
    {
        std::cout << "[CHANNEL] upsert failed: getChannelInfo verify failed\n";
        MeshDB::MarkChannelSyncErrorByKeyHex(rec.keyHex, "getChannelInfo verify failed");
        return false;
    }

    if (info->isEmpty())
    {
        std::cout << "[CHANNEL] upsert failed: verify returned empty slot\n";
        MeshDB::MarkChannelSyncErrorByKeyHex(rec.keyHex, "verify returned empty slot");
        return false;
    }

    if (!MeshDB::UpsertLocalChannel(
            info->channelIdx,
            info->name,
            rec.joinMode,
            rec.passphrase,
            Hex16(info->secret),
            rec.enabled,
            rec.isDefault))
    {
        std::cout << "[CHANNEL] upsert failed: db upsert after verify failed\n";
        MeshDB::MarkChannelSyncErrorByKeyHex(rec.keyHex, "db upsert after verify failed");
        return false;
    }

    std::cout
        << "[CHANNEL] synced upsert idx=" << unsigned(info->channelIdx)
        << " name=\"" << info->name << "\"\n";

    return true;
}

bool AppRuntime::ApplyPendingChannelDelete(const MeshDB::ChannelRecord& rec)
{
    if (rec.channelIdx == 0)
    {
        MeshDB::MarkChannelSyncErrorByKeyHex(rec.keyHex, "refusing to delete default channel");
        return false;
    }

    if (!m_client.deleteChannel(rec.channelIdx))
    {
        MeshDB::MarkChannelSyncErrorByKeyHex(rec.keyHex, "deleteChannel failed");
        return false;
    }

    auto info = m_client.getChannelInfo(rec.channelIdx);
    if (!info.has_value())
    {
        MeshDB::MarkChannelSyncErrorByKeyHex(rec.keyHex, "getChannelInfo verify failed");
        return false;
    }

    if (!info->isEmpty())
    {
        MeshDB::MarkChannelSyncErrorByKeyHex(rec.keyHex, "slot still not empty after delete");
        return false;
    }

    if (!MeshDB::DeleteChannelByKeyHex(rec.keyHex))
    {
        MeshDB::MarkChannelSyncErrorByKeyHex(rec.keyHex, "db delete failed");
        return false;
    }

    std::cout
        << "[CHANNEL] synced delete idx=" << unsigned(rec.channelIdx)
        << " key=" << rec.keyHex
        << "\n";

    return true;
}

void AppRuntime::ProcessCompanionActions()
{
    while (true)
    {
        auto actionOpt = MeshDB::FetchNextQueuedCompanionAction();

        if (!actionOpt.has_value())
        {
            break;
        }

        if (!MeshDB::MarkCompanionActionRunning(actionOpt->id))
        {
            break;
        }

        if (!ProcessSingleCompanionAction(*actionOpt))
        {
            break;
        }
    }
}

bool AppRuntime::ProcessSingleCompanionAction(const MeshDB::CompanionAction& action)
{
    if (action.actionType == "reset_path")
    {
        std::array<uint8_t, 32> publicKey {};

        if (!ParseHex32Local(action.publicKeyHex, publicKey))
        {
            MeshDB::MarkCompanionActionFailed(action.id, "invalid public_key_hex");
            return false;
        }

        if (!m_client.resetPath(publicKey))
        {
            MeshDB::MarkCompanionActionFailed(action.id, "resetPath failed");
            return false;
        }

        if (!MeshDB::MarkCompanionActionDone(action.id))
        {
            return false;
        }

        std::cout
            << "[companion] reset_path done for "
            << action.publicKeyHex
            << "\n";

        return true;
    }

    MeshDB::MarkCompanionActionFailed(action.id, "unknown action_type");
    return false;
}

void AppRuntime::PollRadioStatus()
{
    const auto now = std::chrono::steady_clock::now();

    if (now < m_nextRadioStatusPollAt)
    {
        return;
    }

    m_nextRadioStatusPollAt = now + std::chrono::seconds(RADIO_STATUS_POLL_SECONDS);

    auto stats = m_client.getRadioStats();

    if (!stats.has_value())
    {
        std::cerr << "[radio_status] getRadioStats() failed\n";
        return;
    }

    std::ostringstream oss;
    oss << "{"
        << "\"noise_floor\":" << stats->noiseFloor << ","
        << "\"last_rssi\":" << stats->lastRssi << ","
        << "\"last_snr\":" << std::fixed << std::setprecision(1) << stats->lastSnr << ","
        << "\"tx_air_secs\":" << stats->txAirSecs << ","
        << "\"rx_air_secs\":" << stats->rxAirSecs
        << "}";

    if (!MeshDB::StoreCompanionRadioStatusJson(oss.str()))
    {
        std::cerr << "[radio_status] database update failed\n";
    }
}
