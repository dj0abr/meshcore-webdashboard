#ifndef MESHDB_H
#define MESHDB_H

#include "DataConnector.h"

#include <mariadb/mysql.h>

#include <array>
#include <cstdint>
#include <ctime>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

class MeshDB
{
public:

    struct Config
    {
        std::string host = "localhost";
        std::string user = "meshcore";
        std::string password = "";
        std::string database = "meshcore";
        std::string socketPath = "/run/mysqld/mysqld.sock";
        unsigned int port = 0;
        bool useUnixSocket = true;
    };

    enum class TxStatus : uint8_t
    {
        Queued = 0,
        WaitingAck = 1,
        Failed = 2,
        Done = 3
    };

    struct OutgoingTx
    {
        enum class Kind : uint8_t
        {
            Direct = 0,
            Room = 1,
            FloodAdvert = 2,
            Channel = 3
        };

        unsigned long long id = 0;
        Kind kind = Kind::Direct;

        std::string targetName;
        uint32_t targetNodeId = 0;

        std::string roomName;
        uint32_t roomNodeId = 0;

        uint8_t channelIdx = 0;
        std::string channelName;
        std::string channelKeyHex;

        std::string messageText;
        uint8_t retryCount = 0;
        uint8_t maxRetries = 3;
        uint32_t senderTimestamp = 0;
        std::string currentAckHex;
        uint32_t suggestedTimeoutMs = 0;
    };

    struct ChannelRecord
    {
        uint8_t channelIdx = 0;
        std::string name;
        uint8_t joinMode = 0; // 0=manual,1=hashtag,2=name_passphrase
        std::string passphrase;
        std::string keyHex;
        bool enabled = true;
        bool isDefault = false;
        bool isObserved = false;
        bool hasLocalContext = false;
        bool syncPending = false;
        std::string syncAction;
        std::string syncError;
        uint32_t lastSeenEpoch = 0;
    };    
    struct CompanionConfig
    {
        std::string name;
        int32_t latitudeE6 = 0;
        int32_t longitudeE6 = 0;
        uint32_t radioBwHz = 62500;
        uint8_t radioSf = 8;
        uint8_t radioCr = 8;
        bool applyPending = false;
        std::string lastError;
    };

        enum class DiscoverStatus : uint8_t
    {
        Queued = 0,
        Running = 1,
        Failed = 2,
        Done = 3,
        Skipped = 4
    };

    struct DiscoverJob
    {
        unsigned long long id = 0;
        uint8_t typeFilter = 0;
        uint32_t requestTag = 0;
        uint32_t resultCount = 0;
        std::string requestedBy;
        std::string errorText;
    };

    struct DiscoverResultRow
    {
        unsigned long long lastJobId = 0;
        std::string nodeIdHex;
        float snrRxDb = 0.0f;
        float snrTxDb = 0.0f;
        int rssiDbm = 0;
        uint8_t sourceCode = 0;
    };

    static bool CreateDiscoverJob(
        uint8_t typeFilter,
        const std::string& requestedBy,
        unsigned long long* insertedId = nullptr);

    static std::optional<DiscoverJob> FetchNextQueuedDiscoverJob();

    static bool MarkDiscoverJobRunning(
        unsigned long long id,
        uint32_t requestTag);

    static bool InsertDiscoverResult(
        const DiscoverResultRow& row);

    static bool MarkDiscoverJobDone(
        unsigned long long id);

    static bool MarkDiscoverJobFailed(
        unsigned long long id,
        const std::string& errorText);

    static bool Init(const Config& config);
    static void Shutdown();
    static bool IsReady();

    static bool StoreAdvert(const DataConnector::AdvertInfo& info, const std::string& summary);
    static bool StoreMessage(const DataConnector::MessageInfo& info, const std::string& summary);

    static bool StorePushAdvert(const DataConnector::PushAdvertInfo& info, const std::string& summary);
    static bool StorePushPathUpdated(const DataConnector::PushPathUpdatedInfo& info, const std::string& summary);
    static bool StorePushNewAdvert(const DataConnector::PushNewAdvertInfo& info, const std::string& summary);
    static bool StorePushUnknown(const DataConnector::PushUnknownInfo& info, const std::string& summary);

    static std::optional<OutgoingTx> FetchNextQueuedTx();
    static std::vector<OutgoingTx> FetchTimedOutWaitingTx(unsigned int limit);

    static bool ClearAllTables();
    static bool ClearNodesTable();
    static bool ClearTxBoxTable();

    static bool MarkTxWaitingAck(
        unsigned long long id,
        uint32_t resolvedNodeId,
        uint32_t senderTimestamp,
        uint8_t retryCount,
        uint32_t ack,
        uint32_t suggestedTimeoutMs);

    static bool RequeueTx(
        unsigned long long id,
        uint8_t newRetryCount,
        const std::string& errorText,
        uint32_t nextAttemptEpoch);

    static bool MarkTxDeferred(
        unsigned long long id,
        const std::string& errorText,
        uint32_t nextAttemptEpoch);

    static bool MarkTxFailed(
        unsigned long long id,
        const std::string& errorText);

    static bool DeleteTxByAck(
        uint32_t ack,
        uint32_t rttMs);

    static bool MarkTxDone(unsigned long long id);

    static std::optional<uint32_t> FindRoomNodeIdByName(const std::string& roomName);
    static std::optional<std::string> FindNodeNameByPrefix4(const std::array<uint8_t, 4>& prefix4);
    static bool UpdateTxRoomNodeId(unsigned long long id, uint32_t roomNodeId);
    static bool UpsertRoomPassword(
        uint32_t roomNodeId,
        const std::string& roomName,
        const std::string& password);
    static std::optional<std::string> FindRoomPassword(uint32_t roomNodeId);
    static bool DeleteRoomPassword(uint32_t roomNodeId);

    static bool UpsertLocalChannel(
        uint8_t channelIdx,
        const std::string& name,
        uint8_t joinMode,
        const std::string& passphrase,
        const std::string& keyHex,
        bool enabled,
        bool isDefault);

    static std::optional<ChannelRecord> FindChannelByIdx(uint8_t channelIdx);
    static std::optional<ChannelRecord> FindChannelByIdxUnlockedImpl(uint8_t channelIdx);
    static std::optional<ChannelRecord> FindChannelByKeyHex(const std::string& keyHex);
    static std::optional<ChannelRecord> FindChannelByName(const std::string& channelName);
    static std::vector<ChannelRecord> ListChannels(bool includeObserved);
    static bool MarkChannelObserved(uint8_t channelIdx);
    static bool MarkChannelObservedUnlocked(uint8_t channelIdx);
    static bool DeleteChannel(uint8_t channelIdx);
    static bool DeleteChannelByKeyHex(const std::string& keyHex);
    static bool ClearChannelsTable();

    static bool SaveCompanionConfig(
                const std::string& name,
                int32_t latitudeE6,
                int32_t longitudeE6,
                uint32_t radioBwHz,
                uint8_t radioSf,
                uint8_t radioCr);

    static std::optional<CompanionConfig> LoadCompanionConfig();
    static bool MarkCompanionConfigApplied();
    static bool MarkCompanionConfigApplyFailed(const std::string& error);
    static bool UpdateDiscoverJobResultCount(
            unsigned long long id,
            uint32_t resultCount);
    static bool CleanupOldDiscoverJobs(
            unsigned keepLastN);
    static bool HasRecentDiscoverJob(unsigned cooldownSeconds);
    static std::optional<DiscoverJob> FetchLatestQueuedDiscoverJob();
    static bool SkipOlderQueuedDiscoverJobs(unsigned long long keepJobId);
    static std::string ResolveChannelDisplayName(uint8_t channelIdx);  
    static std::vector<ChannelRecord> ListPendingChannelSync();
    static bool MarkChannelSyncError(uint8_t channelIdx, const std::string& errorText);
    static bool MarkChannelSyncErrorByKeyHex(const std::string& keyHex, const std::string& errorText);
    static bool MarkChannelDeletePending(uint8_t channelIdx);
    static bool MarkChannelDeletePendingByKeyHex(const std::string& keyHex);
    static bool StorePushRxLog(const DataConnector::PushRxLogInfo& info, const std::string& summary);
    static std::vector<std::string> ListNodeNamesMissingAdvertLocation();
    static bool UpdateNodeAdvertLocationByName(
        const std::string& name,
        int32_t advLatE6,
        int32_t advLonE6);

private:

    static bool ConnectServer();
    static bool ConnectDatabase();
    static bool EnsureDatabaseExists();
    static bool EnsureSchema();
    static bool Execute(const std::string& sql);
    static std::string Escape(const std::string& value);

    static std::string ToSqlString(const std::string& value);
    static std::string ToSqlNullableString(const std::string& value);
    static std::string ToSqlDateTime(std::time_t epoch);
    static std::string ToSqlDateTimeFromU32(uint32_t epoch);
    static std::string BoolToSql(bool value);
    static std::string SanitizeUtf8(const std::string& value);
    static bool MessageAlreadyStoredUnlocked(
            const DataConnector::MessageInfo& info,
            const std::string& prefix6Hex);

    static bool UpsertNodeFromAdvert(const DataConnector::AdvertInfo& info);
    static bool UpsertNodeFromPushAdvert(const DataConnector::PushAdvertInfo& info);
    static bool UpsertNodeFromPushNewAdvert(const DataConnector::PushNewAdvertInfo& info);
    static bool UpsertNodeFromPathUpdated(const DataConnector::PushPathUpdatedInfo& info);

    static MYSQL* s_conn;
    static Config s_config;
    static bool s_ready;
    static std::mutex s_mutex;

    MeshDB() = delete;
    ~MeshDB() = delete;
};

#endif