#include "MeshDB.h"
#include <iostream>
#include <sstream>
#include <cmath>
#include <cstdlib>
#include <optional>
#include <vector>
#include <ctime>
#include <cstdlib>
#include <optional>

MYSQL* MeshDB::s_conn = nullptr;
MeshDB::Config MeshDB::s_config {};
bool MeshDB::s_ready = false;
std::mutex MeshDB::s_mutex;

namespace
{
    std::string RowString(MYSQL_ROW row, unsigned int idx)
    {
        return (row[idx] != nullptr) ? row[idx] : "";
    }

    uint32_t RowU32(MYSQL_ROW row, unsigned int idx)
    {
        return (row[idx] != nullptr)
            ? static_cast<uint32_t>(std::strtoul(row[idx], nullptr, 10))
            : 0U;
    }

    uint8_t RowU8(MYSQL_ROW row, unsigned int idx)
    {
        return (row[idx] != nullptr)
            ? static_cast<uint8_t>(std::strtoul(row[idx], nullptr, 10))
            : 0U;
    }

    unsigned long long RowU64(MYSQL_ROW row, unsigned int idx)
    {
        return (row[idx] != nullptr)
            ? std::strtoull(row[idx], nullptr, 10)
            : 0ULL;
    }

    uint32_t NextTimeoutEpoch(uint32_t suggestedTimeoutMs)
    {
        uint32_t ms = suggestedTimeoutMs + 300;

        if (ms < 800)
        {
            ms = 800;
        }

        if (ms > 20000)
        {
            ms = 20000;
        }

        uint32_t sec = ms / 1000;

        if ((ms % 1000) != 0)
        {
            sec++;
        }

        if (sec == 0)
        {
            sec = 1;
        }

        return static_cast<uint32_t>(std::time(nullptr)) + sec;
    }
}

namespace
{
    MeshDB::OutgoingTx::Kind TxKindFromU32(uint32_t value)
    {
        switch (value)
        {
            case 0:
            {
                return MeshDB::OutgoingTx::Kind::Direct;
            }

            case 1:
            {
                return MeshDB::OutgoingTx::Kind::Room;
            }

            case 2:
            {
                return MeshDB::OutgoingTx::Kind::FloodAdvert;
            }

            case 3:
            {
                return MeshDB::OutgoingTx::Kind::Channel;
            }

            default:
            {
                return MeshDB::OutgoingTx::Kind::Direct;
            }
        }
    }
}

namespace
{
    MeshDB::OutgoingTx BuildOutgoingTxFromRow(MYSQL_ROW row)
    {
        MeshDB::OutgoingTx tx {};

        tx.id = RowU64(row, 0);
        tx.kind = TxKindFromU32(RowU32(row, 1));
        tx.targetName = RowString(row, 2);
        tx.targetNodeId = RowU32(row, 3);
        tx.roomName = RowString(row, 4);
        tx.roomNodeId = RowU32(row, 5);
        tx.channelName = RowString(row, 6);
        tx.channelIdx = RowU8(row, 7);
        tx.messageText = RowString(row, 8);
        tx.retryCount = RowU8(row, 9);
        tx.maxRetries = RowU8(row, 10);
        tx.senderTimestamp = RowU32(row, 11);
        tx.currentAckHex = RowString(row, 12);
        tx.suggestedTimeoutMs = RowU32(row, 13);

        return tx;
    }
}

static std::string RowStr(MYSQL_ROW row, int idx)
{
    if (row == nullptr)
    {
        return std::string();
    }

    const char* v = row[idx];

    if (v == nullptr)
    {
        return std::string();
    }

    return std::string(v);
}

static int32_t RowI32(MYSQL_ROW row, int idx)
{
    if (row == nullptr)
    {
        return 0;
    }

    const char* v = row[idx];

    if (v == nullptr)
    {
        return 0;
    }

    return static_cast<int32_t>(std::strtol(v, nullptr, 10));
}

static std::optional<std::string> RowNullableStr(MYSQL_ROW row, int idx)
{
    if (row == nullptr)
    {
        return std::nullopt;
    }

    const char* v = row[idx];

    if (v == nullptr)
    {
        return std::nullopt;
    }

    return std::string(v);
}

bool MeshDB::Init(const Config& config)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    s_config = config;
    s_ready = false;

    if (s_conn != nullptr)
    {
        mysql_close(s_conn);
        s_conn = nullptr;
    }

/*
    Einmalige Einrichtung im Terminal:

    sudo mariadb

    Dann in MariaDB:

    DROP USER IF EXISTS 'meshcore'@'localhost';

    CREATE DATABASE IF NOT EXISTS meshcore
    CHARACTER SET utf8mb4
    COLLATE utf8mb4_general_ci;

    CREATE USER 'meshcore'@'localhost';
    GRANT ALL PRIVILEGES ON meshcore.* TO 'meshcore'@'localhost';
    FLUSH PRIVILEGES;

    Diese Konfiguration ist fuer ein rein lokales Einzelplatz-Setup gedacht.
    Verbindung erfolgt ueber localhost / Unix Socket ohne Passwort.
*/

    if (!ConnectServer())
    {
        std::cerr << "MeshDB: Verbindung zum MariaDB-Server fehlgeschlagen.\n";
        return false;
    }

    if (!EnsureDatabaseExists())
    {
        std::cerr << "MeshDB: Datenbank konnte nicht angelegt werden.\n";
        return false;
    }

    mysql_close(s_conn);
    s_conn = nullptr;

    if (!ConnectDatabase())
    {
        std::cerr << "MeshDB: Verbindung zur Datenbank fehlgeschlagen.\n";
        return false;
    }

    if (!EnsureSchema())
    {
        std::cerr << "MeshDB: Schema konnte nicht erstellt werden.\n";
        return false;
    }

    s_ready = true;
    return true;
}

void MeshDB::Shutdown()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (s_conn != nullptr)
    {
        mysql_close(s_conn);
        s_conn = nullptr;
    }

    s_ready = false;
}

bool MeshDB::IsReady()
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return s_ready && (s_conn != nullptr);
}

bool MeshDB::ConnectServer()
{
    s_conn = mysql_init(nullptr);

    if (s_conn == nullptr)
    {
        return false;
    }

    const char* hostPtr = s_config.useUnixSocket ? "localhost" : s_config.host.c_str();
    const char* passwordPtr = s_config.password.empty() ? "" : s_config.password.c_str();
    const char* socketPtr = s_config.useUnixSocket ? s_config.socketPath.c_str() : nullptr;
    unsigned int port = s_config.useUnixSocket ? 0 : s_config.port;

    if (mysql_real_connect(
            s_conn,
            hostPtr,
            s_config.user.c_str(),
            passwordPtr,
            nullptr,
            port,
            socketPtr,
            0) == nullptr)
    {
        std::cerr << "MeshDB: mysql_real_connect(server) failed: "
                  << mysql_error(s_conn) << "\n";
        mysql_close(s_conn);
        s_conn = nullptr;
        return false;
    }

    if (mysql_set_character_set(s_conn, "utf8mb4") != 0)
    {
        std::cerr << "MeshDB: mysql_set_character_set failed: "
                << mysql_error(s_conn) << "\n";
        mysql_close(s_conn);
        s_conn = nullptr;
        return false;
    }

    if (!Execute("SET NAMES utf8mb4 COLLATE utf8mb4_unicode_ci"))
    {
        mysql_close(s_conn);
        s_conn = nullptr;
        return false;
    }
    return true;
}

bool MeshDB::ConnectDatabase()
{
    s_conn = mysql_init(nullptr);

    if (s_conn == nullptr)
    {
        return false;
    }

    const char* hostPtr = s_config.useUnixSocket ? "localhost" : s_config.host.c_str();
    const char* passwordPtr = s_config.password.empty() ? "" : s_config.password.c_str();
    const char* socketPtr = s_config.useUnixSocket ? s_config.socketPath.c_str() : nullptr;
    unsigned int port = s_config.useUnixSocket ? 0 : s_config.port;

    if (mysql_real_connect(
            s_conn,
            hostPtr,
            s_config.user.c_str(),
            passwordPtr,
            s_config.database.c_str(),
            port,
            socketPtr,
            0) == nullptr)
    {
        std::cerr << "MeshDB: mysql_real_connect(database) failed: "
                  << mysql_error(s_conn) << "\n";
        mysql_close(s_conn);
        s_conn = nullptr;
        return false;
    }

    if (mysql_set_character_set(s_conn, "utf8mb4") != 0)
    {
        std::cerr << "MeshDB: mysql_set_character_set failed: "
                << mysql_error(s_conn) << "\n";
        mysql_close(s_conn);
        s_conn = nullptr;
        return false;
    }

    if (!Execute("SET NAMES utf8mb4 COLLATE utf8mb4_unicode_ci"))
    {
        mysql_close(s_conn);
        s_conn = nullptr;
        return false;
    }
    return true;
}

bool MeshDB::EnsureDatabaseExists()
{
    std::ostringstream oss;
    oss << "CREATE DATABASE IF NOT EXISTS `" << s_config.database
        << "` CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci";

    return Execute(oss.str());
}

bool MeshDB::EnsureSchema()
{
    const char* sqlNodes =
        "CREATE TABLE IF NOT EXISTS nodes ("
        "    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,"
        "    node_id INT UNSIGNED DEFAULT NULL,"
        "    advert_type TINYINT UNSIGNED NOT NULL DEFAULT 0,"
        "    advert_flags TINYINT UNSIGNED NOT NULL DEFAULT 0,"
        "    name VARCHAR(64) NOT NULL DEFAULT '',"
        "    public_key_hex CHAR(64) DEFAULT NULL,"
        "    prefix6_hex CHAR(12) DEFAULT NULL,"
        "    adv_lat_e6 INT DEFAULT NULL,"
        "    adv_lon_e6 INT DEFAULT NULL,"
        "    last_advert_at DATETIME DEFAULT NULL,"
        "    last_mod_at DATETIME DEFAULT NULL,"
        "    first_seen_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
        "    PRIMARY KEY (id),"
        "    UNIQUE KEY uq_nodes_node_id (node_id),"
        "    UNIQUE KEY uq_nodes_public_key_hex (public_key_hex),"
        "    UNIQUE KEY uq_nodes_prefix6_hex (prefix6_hex),"
        "    KEY idx_nodes_name (name),"
        "    KEY idx_nodes_last_advert_at (last_advert_at)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

    const char* sqlMessages =
        "CREATE TABLE IF NOT EXISTS messages ("
        "    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,"
        "    received_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "    is_channel TINYINT(1) NOT NULL,"
        "    channel_idx INT UNSIGNED DEFAULT NULL,"
        "    from_name VARCHAR(64) NOT NULL,"
        "    room_sender_name VARCHAR(64) DEFAULT NULL,"
        "    sender_prefix6_hex CHAR(12) DEFAULT NULL,"
        "    sender_timestamp INT UNSIGNED DEFAULT NULL,"
        "    snr_db FLOAT DEFAULT NULL,"
        "    path_len TINYINT UNSIGNED DEFAULT NULL,"
        "    txt_type TINYINT UNSIGNED DEFAULT NULL,"
        "    message_text TEXT NOT NULL,"
        "    PRIMARY KEY (id),"
        "    KEY idx_messages_received_at (received_at),"
        "    KEY idx_messages_is_channel (is_channel),"
        "    KEY idx_messages_channel_idx (channel_idx),"
        "    KEY idx_messages_sender_prefix6_hex (sender_prefix6_hex)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

    const char* sqlEvents =
        "CREATE TABLE IF NOT EXISTS events ("
        "    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,"
        "    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "    event_type TINYINT UNSIGNED NOT NULL,"
        "    event_name VARCHAR(32) NOT NULL,"
        "    name VARCHAR(64) DEFAULT NULL,"
        "    prefix6_hex CHAR(12) DEFAULT NULL,"
        "    public_key_hex CHAR(64) DEFAULT NULL,"
        "    ack_hex CHAR(8) DEFAULT NULL,"
        "    tag_hex CHAR(8) DEFAULT NULL,"
        "    auth_code_hex CHAR(8) DEFAULT NULL,"
        "    payload_len INT DEFAULT NULL,"
        "    rtt_ms INT DEFAULT NULL,"
        "    flags INT DEFAULT NULL,"
        "    is_valid TINYINT(1) DEFAULT NULL,"
        "    adv_lat_e6 INT DEFAULT NULL,"
        "    adv_lon_e6 INT DEFAULT NULL,"
        "    summary TEXT NOT NULL DEFAULT '',"
        "    PRIMARY KEY (id),"
        "    KEY idx_events_created_at (created_at),"
        "    KEY idx_events_event_type (event_type),"
        "    KEY idx_events_prefix6_hex (prefix6_hex),"
        "    KEY idx_events_public_key_hex (public_key_hex)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

    const char* sqlTraceHops =
        "CREATE TABLE IF NOT EXISTS event_trace_hops ("
        "    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,"
        "    event_id BIGINT UNSIGNED NOT NULL,"
        "    hop_index SMALLINT UNSIGNED NOT NULL,"
        "    path_hash_hex CHAR(2) NOT NULL,"
        "    snr_db FLOAT DEFAULT NULL,"
        "    PRIMARY KEY (id),"
        "    UNIQUE KEY uq_event_trace_hop (event_id, hop_index),"
        "    CONSTRAINT fk_event_trace_hops_event "
        "        FOREIGN KEY (event_id) REFERENCES events(id)"
        "        ON DELETE CASCADE"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

    const char* sqlTxOutbox =
        "CREATE TABLE IF NOT EXISTS tx_outbox ("
        "    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,"
        "    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
        "    tx_kind TINYINT UNSIGNED NOT NULL COMMENT '0=direct,1=room,2=floodAdvert,3=channel',"
        "    target_name VARCHAR(64) DEFAULT NULL,"
        "    target_node_id INT UNSIGNED DEFAULT NULL,"
        "    room_name VARCHAR(64) DEFAULT NULL,"
        "    room_node_id INT UNSIGNED DEFAULT NULL,"
        "    channel_name VARCHAR(64) DEFAULT NULL,"
        "    channel_idx INT UNSIGNED DEFAULT NULL,"
        "    message_text TEXT NOT NULL,"
        "    status TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=queued,1=waiting_ack,2=failed,3=done',"
        "    retry_count TINYINT UNSIGNED NOT NULL DEFAULT 0,"
        "    max_retries TINYINT UNSIGNED NOT NULL DEFAULT 3,"
        "    sender_timestamp INT UNSIGNED DEFAULT NULL,"
        "    current_ack_hex CHAR(8) DEFAULT NULL,"
        "    suggested_timeout_ms INT UNSIGNED DEFAULT NULL,"
        "    last_attempt_epoch INT UNSIGNED DEFAULT NULL,"
        "    last_rtt_ms INT UNSIGNED DEFAULT NULL,"
        "    timeout_epoch INT UNSIGNED DEFAULT NULL,"
        "    next_attempt_epoch INT UNSIGNED NOT NULL DEFAULT 0,"
        "    resolved_node_id INT UNSIGNED DEFAULT NULL,"
        "    last_error VARCHAR(255) DEFAULT NULL,"
        "    PRIMARY KEY (id),"
        "    KEY idx_tx_outbox_status_next_attempt (status, next_attempt_epoch),"
        "    KEY idx_tx_outbox_timeout_epoch (timeout_epoch),"
        "    KEY idx_tx_outbox_current_ack_hex (current_ack_hex),"
        "    KEY idx_tx_outbox_target_node_id (target_node_id),"
        "    KEY idx_tx_outbox_room_node_id (room_node_id),"
        "    KEY idx_tx_outbox_channel_name (channel_name),"
        "    KEY idx_tx_outbox_channel_idx (channel_idx)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
        
    const char* sqlRoomCredentials =
        "CREATE TABLE IF NOT EXISTS room_credentials ("
        "    room_node_id INT UNSIGNED NOT NULL,"
        "    room_name VARCHAR(64) DEFAULT NULL,"
        "    password VARCHAR(64) NOT NULL,"
        "    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP "
        "        ON UPDATE CURRENT_TIMESTAMP,"
        "    PRIMARY KEY (room_node_id),"
        "    KEY idx_room_credentials_name (room_name)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

        const char* sqlChannels =
        "CREATE TABLE IF NOT EXISTS channels ("
        "    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,"
        "    channel_idx INT UNSIGNED NOT NULL,"
        "    name VARCHAR(64) NOT NULL,"
        "    join_mode TINYINT UNSIGNED NOT NULL DEFAULT 0,"
        "    passphrase VARCHAR(128) DEFAULT NULL,"
        "    key_hex CHAR(64) DEFAULT NULL,"
        "    enabled TINYINT(1) NOT NULL DEFAULT 1,"
        "    is_default TINYINT(1) NOT NULL DEFAULT 0,"
        "    is_observed TINYINT(1) NOT NULL DEFAULT 0,"
        "    has_local_context TINYINT(1) NOT NULL DEFAULT 0,"
        "    sync_pending TINYINT(1) NOT NULL DEFAULT 0,"
        "    sync_action VARCHAR(16) NOT NULL DEFAULT '',"
        "    sync_error VARCHAR(255) NOT NULL DEFAULT '',"
        "    last_seen_at DATETIME DEFAULT NULL,"
        "    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
        "    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
        "    PRIMARY KEY (id),"
        "    UNIQUE KEY uq_channels_idx (channel_idx),"
        "    KEY idx_channels_name (name),"
        "    KEY idx_channels_observed (is_observed),"
        "    KEY idx_channels_enabled (enabled),"
        "    KEY idx_channels_sync_pending (sync_pending),"
        "    KEY idx_channels_sync_action (sync_action),"
        "    KEY idx_channels_local_context (has_local_context)"
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
        
        const char* sqlChatMessages =
            "CREATE TABLE IF NOT EXISTS chat_messages ("
            "    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,"
            "    timestamp_epoch INT UNSIGNED NOT NULL,"
            "    direction TINYINT UNSIGNED NOT NULL COMMENT '0=rx,1=tx',"
            "    chat_kind TINYINT UNSIGNED NOT NULL COMMENT '0=dm,1=room,2=channel',"
            "    name VARCHAR(64) NOT NULL,"
            "    room_sender_name VARCHAR(64) DEFAULT NULL,"
            "    channel_idx INT UNSIGNED DEFAULT NULL,"
            "    peer_node_id INT UNSIGNED DEFAULT NULL,"
            "    room_node_id INT UNSIGNED DEFAULT NULL,"
            "    text TEXT NOT NULL,"
            "    status TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=received,1=queued,2=sent,3=failed',"
            "    tx_outbox_id BIGINT UNSIGNED DEFAULT NULL,"
            "    sender_prefix6_hex CHAR(12) DEFAULT NULL,"
            "    snr_db FLOAT DEFAULT NULL,"
            "    path_len TINYINT UNSIGNED DEFAULT NULL,"
            "    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "    PRIMARY KEY (id),"
            "    KEY idx_chat_messages_conv (chat_kind, name, timestamp_epoch),"
            "    KEY idx_chat_messages_channel (channel_idx, timestamp_epoch),"
            "    KEY idx_chat_messages_tx_outbox_id (tx_outbox_id)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";

        const char* sqlCompanionConfig =
            "CREATE TABLE IF NOT EXISTS companion_config ("
            "    id TINYINT UNSIGNED NOT NULL,"
            "    name VARCHAR(64) NOT NULL,"
            "    latitude_e6 INT NOT NULL,"
            "    longitude_e6 INT NOT NULL,"
            "    radio_bw_hz INT UNSIGNED NOT NULL,"
            "    radio_sf TINYINT UNSIGNED NOT NULL,"
            "    radio_cr TINYINT UNSIGNED NOT NULL,"
            "    apply_pending TINYINT(1) NOT NULL DEFAULT 1,"
            "    last_applied_at DATETIME DEFAULT NULL,"
            "    last_error VARCHAR(255) DEFAULT NULL,"
            "    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
            "    PRIMARY KEY (id)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
            
        const char* sqlDiscoverJobs =
            "CREATE TABLE IF NOT EXISTS discover_jobs ("
            "    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,"
            "    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
            "    started_at DATETIME DEFAULT NULL,"
            "    finished_at DATETIME DEFAULT NULL,"
            "    status TINYINT UNSIGNED NOT NULL DEFAULT 0 COMMENT '0=queued,1=running,2=failed,3=done,4=skipped',"
            "    type_filter TINYINT UNSIGNED NOT NULL,"
            "    request_tag INT UNSIGNED DEFAULT NULL,"
            "    requested_by VARCHAR(64) DEFAULT NULL,"
            "    result_count INT UNSIGNED NOT NULL DEFAULT 0,"
            "    error_text VARCHAR(255) DEFAULT NULL,"
            "    PRIMARY KEY (id),"
            "    KEY idx_discover_jobs_status_created (status, created_at),"
            "    KEY idx_discover_jobs_created_at (created_at)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
    const char* sqlDiscoverResults =
            "CREATE TABLE IF NOT EXISTS discover_results ("
            "    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,"
            "    last_job_id BIGINT UNSIGNED DEFAULT NULL,"
            "    created_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,"
            "    updated_at DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
            "    node_id_hex CHAR(16) NOT NULL,"
            "    snr_rx_db FLOAT NOT NULL,"
            "    snr_tx_db FLOAT NOT NULL,"
            "    rssi_dbm INT NOT NULL,"
            "    source_code TINYINT UNSIGNED NOT NULL,"
            "    PRIMARY KEY (id),"
            "    UNIQUE KEY uq_discover_results_node (node_id_hex),"
            "    KEY idx_discover_results_last_job_id (last_job_id),"
            "    KEY idx_discover_results_rssi (rssi_dbm)"
            ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4";
            
    return Execute(sqlNodes)
        && Execute(sqlMessages)
        && Execute(sqlEvents)
        && Execute(sqlTraceHops)
        && Execute(sqlTxOutbox)
        && Execute(sqlRoomCredentials)
        && Execute(sqlChannels)
        && Execute(sqlChatMessages)
        && Execute(sqlCompanionConfig)
        && Execute(sqlDiscoverJobs)
        && Execute(sqlDiscoverResults);
}

bool MeshDB::Execute(const std::string& sql)
{
    if (s_conn == nullptr)
    {
        return false;
    }

    if (mysql_query(s_conn, sql.c_str()) != 0)
    {
        std::cerr << "MeshDB SQL error: " << mysql_error(s_conn) << "\n";
        std::cerr << "SQL: " << sql << "\n";
        return false;
    }

    return true;
}

std::string MeshDB::Escape(const std::string& value)
{
    if (s_conn == nullptr)
    {
        return value;
    }

    std::string out;
    out.resize(value.size() * 2 + 1);

    unsigned long n = mysql_real_escape_string(
        s_conn,
        &out[0],
        value.c_str(),
        static_cast<unsigned long>(value.size()));

    out.resize(n);
    return out;
}

std::string MeshDB::ToSqlString(const std::string& value)
{
    const std::string clean = SanitizeUtf8(value);
    return "'" + Escape(clean) + "'";
}

std::string MeshDB::ToSqlNullableString(const std::string& value)
{
    if (value.empty())
    {
        return "NULL";
    }

    const std::string clean = SanitizeUtf8(value);
    return "'" + Escape(clean) + "'";
}

std::string MeshDB::ToSqlDateTime(std::time_t epoch)
{
    if (epoch <= 0)
    {
        return "NULL";
    }

    return "'" + Escape(DataConnector::formatLocalTime(static_cast<uint32_t>(epoch))) + "'";
}

std::string MeshDB::ToSqlDateTimeFromU32(uint32_t epoch)
{
    if (epoch == 0)
    {
        return "NULL";
    }

    return "'" + Escape(DataConnector::formatLocalTime(epoch)) + "'";
}

std::string MeshDB::BoolToSql(bool value)
{
    return value ? "1" : "0";
}

bool MeshDB::InsertEvent(
    uint8_t eventType,
    const std::string& eventName,
    const std::string& summary,
    const std::string& name,
    const std::string& prefix6Hex,
    const std::string& publicKeyHex,
    const std::string& ackHex,
    const std::string& tagHex,
    const std::string& authCodeHex,
    int payloadLen,
    int rttMs,
    int flags,
    int isValid,
    int advLatE6,
    int advLonE6,
    unsigned long long* insertedId)
{
    std::ostringstream oss;

    oss
        << "INSERT INTO events ("
        << "event_type, event_name, name, prefix6_hex, public_key_hex, ack_hex, "
        << "tag_hex, auth_code_hex, payload_len, rtt_ms, flags, is_valid, "
        << "adv_lat_e6, adv_lon_e6, summary"
        << ") VALUES ("
        << unsigned(eventType) << ", "
        << ToSqlString(eventName) << ", "
        << ToSqlNullableString(name) << ", "
        << ToSqlNullableString(prefix6Hex) << ", "
        << ToSqlNullableString(publicKeyHex) << ", "
        << ToSqlNullableString(ackHex) << ", "
        << ToSqlNullableString(tagHex) << ", "
        << ToSqlNullableString(authCodeHex) << ", "
        << ((payloadLen >= 0) ? std::to_string(payloadLen) : "NULL") << ", "
        << ((rttMs >= 0) ? std::to_string(rttMs) : "NULL") << ", "
        << ((flags >= 0) ? std::to_string(flags) : "NULL") << ", "
        << ((isValid >= 0) ? std::to_string(isValid) : "NULL") << ", "
        << (((advLatE6 != 0) || (advLonE6 != 0)) ? std::to_string(advLatE6) : "NULL") << ", "
        << (((advLatE6 != 0) || (advLonE6 != 0)) ? std::to_string(advLonE6) : "NULL") << ", "
        << ToSqlString(summary)
        << ")";

    if (!Execute(oss.str()))
    {
        return false;
    }

    if (insertedId != nullptr)
    {
        *insertedId = mysql_insert_id(s_conn);
    }

    return true;
}

bool MeshDB::UpsertNodeFromAdvert(const DataConnector::AdvertInfo& info)
{
    const std::string publicKeyHex =
        DataConnector::hexBytes(info.publicKey.data(), info.publicKey.size());

    const std::string prefix6Hex =
        DataConnector::hexBytes(info.publicKey.data(), 6);

    const bool hasLocation = (info.advLatE6 != 0) || (info.advLonE6 != 0);
    const std::time_t lastAdvert = info.lastAdvert;

    std::ostringstream oss;

    oss
        << "INSERT INTO nodes ("
        << "node_id, advert_type, advert_flags, name, public_key_hex, prefix6_hex, last_advert_at, adv_lat_e6, adv_lon_e6"
        << ") VALUES ("
        << info.nodeId << ", "
        << unsigned(info.type) << ", "
        << unsigned(info.flags) << ", "
        << ToSqlString(info.name) << ", "
        << ToSqlString(publicKeyHex) << ", "
        << ToSqlString(prefix6Hex) << ", "
        << ToSqlDateTime(lastAdvert) << ", ";

    if (hasLocation)
    {
        oss << info.advLatE6 << ", " << info.advLonE6;
    }
    else
    {
        oss << "NULL, NULL";
    }

    oss
        << ") ON DUPLICATE KEY UPDATE "
        << "node_id=VALUES(node_id), "
        << "advert_type=VALUES(advert_type), "
        << "advert_flags=VALUES(advert_flags), "
        << "name=VALUES(name), "
        << "public_key_hex=VALUES(public_key_hex), "
        << "prefix6_hex=VALUES(prefix6_hex), "
        << "last_advert_at=VALUES(last_advert_at), "
        << "adv_lat_e6=VALUES(adv_lat_e6), "
        << "adv_lon_e6=VALUES(adv_lon_e6), "
        << "updated_at=CURRENT_TIMESTAMP";

    return Execute(oss.str());
}

bool MeshDB::UpsertNodeFromPushAdvert(const DataConnector::PushAdvertInfo& info)
{
    if (!info.valid)
    {
        return true;
    }

    const std::string prefix6Hex =
        DataConnector::hexBytes(info.prefix6.data(), info.prefix6.size());

    std::ostringstream oss;

    oss
        << "INSERT INTO nodes (prefix6_hex, name) VALUES ("
        << ToSqlString(prefix6Hex) << ", "
        << ToSqlString(info.name)
        << ") ON DUPLICATE KEY UPDATE "
        << "name=VALUES(name), "
        << "updated_at=CURRENT_TIMESTAMP";

    return Execute(oss.str());
}

bool MeshDB::UpsertNodeFromPushNewAdvert(const DataConnector::PushNewAdvertInfo& info)
{
    if (!info.valid)
    {
        return true;
    }

    const std::string publicKeyHex =
        DataConnector::hexBytes(info.publicKey.data(), info.publicKey.size());

    const std::string prefix6Hex =
        DataConnector::hexBytes(info.prefix6.data(), info.prefix6.size());

    const bool hasLocation = (info.advLatE6 != 0) || (info.advLonE6 != 0);

    std::ostringstream oss;

    oss
        << "INSERT INTO nodes ("
        << "node_id, advert_type, advert_flags, name, public_key_hex, prefix6_hex, last_advert_at, last_mod_at, adv_lat_e6, adv_lon_e6"
        << ") VALUES ("
        << info.nodeId << ", "
        << unsigned(info.type) << ", "
        << unsigned(info.flags) << ", "
        << ToSqlString(info.name) << ", "
        << ToSqlString(publicKeyHex) << ", "
        << ToSqlString(prefix6Hex) << ", "
        << ToSqlDateTimeFromU32(info.lastAdvert) << ", "
        << ToSqlDateTimeFromU32(info.lastMod) << ", ";

    if (hasLocation)
    {
        oss << info.advLatE6 << ", " << info.advLonE6;
    }
    else
    {
        oss << "NULL, NULL";
    }

    oss
        << ") ON DUPLICATE KEY UPDATE "
        << "node_id=VALUES(node_id), "
        << "advert_type=VALUES(advert_type), "
        << "advert_flags=VALUES(advert_flags), "
        << "name=VALUES(name), "
        << "public_key_hex=VALUES(public_key_hex), "
        << "prefix6_hex=VALUES(prefix6_hex), "
        << "last_advert_at=VALUES(last_advert_at), "
        << "last_mod_at=VALUES(last_mod_at), "
        << "adv_lat_e6=VALUES(adv_lat_e6), "
        << "adv_lon_e6=VALUES(adv_lon_e6), "
        << "updated_at=CURRENT_TIMESTAMP";

    return Execute(oss.str());
}

bool MeshDB::UpsertNodeFromPathUpdated(const DataConnector::PushPathUpdatedInfo& info)
{
    if (!info.valid)
    {
        return true;
    }

    const std::string publicKeyHex =
        DataConnector::hexBytes(info.publicKey.data(), info.publicKey.size());

    std::ostringstream oss;

    oss
        << "INSERT INTO nodes (public_key_hex) VALUES ("
        << ToSqlString(publicKeyHex)
        << ") ON DUPLICATE KEY UPDATE "
        << "updated_at=CURRENT_TIMESTAMP";

    return Execute(oss.str());
}

bool MeshDB::StoreAdvert(const DataConnector::AdvertInfo& info, const std::string& summary)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    if (!UpsertNodeFromAdvert(info))
    {
        return false;
    }

    const std::string publicKeyHex =
        DataConnector::hexBytes(info.publicKey.data(), info.publicKey.size());

    return InsertEvent(
        static_cast<uint8_t>(DataConnector::EventType::Advert),
        "advert",
        summary,
        info.name,
        "",
        publicKeyHex,
        "",
        "",
        "",
        -1,
        -1,
        static_cast<int>(info.flags),
        1,
        static_cast<int>(info.advLatE6),
        static_cast<int>(info.advLonE6),
        nullptr);
}

bool MeshDB::MessageAlreadyStoredUnlocked(
    const DataConnector::MessageInfo& info,
    const std::string& prefix6Hex)
{
    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    std::ostringstream sql;

    sql
        << "SELECT id FROM messages WHERE "
        << "is_channel = " << BoolToSql(info.isChannel) << " AND ";

    if (info.isChannel)
    {
        sql
            << "channel_idx = " << unsigned(info.channelIdx) << " AND ";
    }
    else
    {
        sql
            << "sender_prefix6_hex = " << ToSqlNullableString(prefix6Hex) << " AND ";
    }

    sql
        << "sender_timestamp = "
        << ((info.senderTimestamp != 0)
            ? std::to_string(info.senderTimestamp)
            : "NULL")
        << " AND "
        << "txt_type = " << unsigned(info.txtType) << " AND "
        << "message_text = " << ToSqlString(info.text)
        << " LIMIT 1";

    if (mysql_query(s_conn, sql.str().c_str()) != 0)
    {
        std::cerr << "MeshDB SQL error: " << mysql_error(s_conn) << "\n";
        return false;
    }

    MYSQL_RES* res = mysql_store_result(s_conn);

    if (res == nullptr)
    {
        return false;
    }

    const bool exists = (mysql_fetch_row(res) != nullptr);
    mysql_free_result(res);

    return exists;
}

bool MeshDB::StoreMessage(const DataConnector::MessageInfo& info, const std::string& summary)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    std::cout << "[MeshDB] StoreMessage:"
          << " kind=" << unsigned(static_cast<uint8_t>(info.kind))
          << " channelIdx=" << unsigned(info.channelIdx)
          << " fromName=[" << info.fromName << "]"
          << " text=[" << info.text << "]"
          << "\n";

    const std::string prefix6Hex =
        DataConnector::hexBytes(info.senderPrefix6.data(), info.senderPrefix6.size());

    if (MessageAlreadyStoredUnlocked(info, prefix6Hex))
    {
        std::cout << "[MeshDB] StoreMessage: duplicate RX ignored\n";
        return true;
    }

    std::ostringstream msgSql;

    msgSql
        << "INSERT INTO messages ("
        << "is_channel, channel_idx, from_name, room_sender_name, "
        << "sender_prefix6_hex, sender_timestamp, snr_db, path_len, txt_type, message_text"
        << ") VALUES ("
        << BoolToSql(info.isChannel) << ", "
        << (info.isChannel ? std::to_string(unsigned(info.channelIdx)) : "NULL") << ", "
        << ToSqlString(info.fromName) << ", "
        << (info.roomSenderName.empty() ? "NULL" : ToSqlString(info.roomSenderName)) << ", "
        << ToSqlNullableString(prefix6Hex) << ", "
        << ((info.senderTimestamp != 0) ? std::to_string(info.senderTimestamp) : "NULL") << ", "
        << (std::isnan(info.snrDb) ? "NULL" : std::to_string(info.snrDb)) << ", "
        << ((info.pathLen == 255) ? "NULL" : std::to_string(unsigned(info.pathLen))) << ", "
        << unsigned(info.txtType) << ", "
        << ToSqlString(info.text)
        << ")";

    if (!Execute(msgSql.str()))
    {
        return false;
    }

    std::cout << "[MeshDB] StoreMessage: inserted into messages\n";
   
    const std::uint64_t nowEpoch =
        static_cast<std::uint64_t>(std::time(nullptr));

    unsigned chatKind = 0U;
    switch (info.kind)
    {
        case DataConnector::MessageInfo::Kind::Direct:
            chatKind = 0U;
            break;

        case DataConnector::MessageInfo::Kind::Room:
            chatKind = 1U;
            break;

        case DataConnector::MessageInfo::Kind::Channel:
            chatKind = 2U;
            break;
    }
    if (info.kind == DataConnector::MessageInfo::Kind::Channel)
    {
        if (!MarkChannelObservedUnlocked(info.channelIdx))
        {
            std::cout << "[MeshDB] StoreMessage: MarkChannelObservedUnlocked FAILED\n";
            return false;
        }
    }
    const unsigned direction = 0U;                        // 0=rx
    const unsigned status = 1U;                           // 1=confirmed

    std::string conversationName;

    if (info.kind == DataConnector::MessageInfo::Kind::Channel)
    {
        conversationName = "ch" + std::to_string(unsigned(info.channelIdx));
    }
    else
    {
        conversationName = info.fromName;
    }

    std::ostringstream chatSql;

    chatSql
        << "INSERT INTO chat_messages ("
        << "timestamp_epoch, direction, chat_kind, name, room_sender_name, channel_idx, peer_node_id, room_node_id, "
        << "text, status, tx_outbox_id, sender_prefix6_hex, snr_db, path_len"
        << ") VALUES ("
        << nowEpoch << ", "
        << direction << ", "
        << chatKind << ", "
        << ToSqlString(conversationName) << ", "
        << (info.roomSenderName.empty() ? "NULL" : ToSqlString(info.roomSenderName)) << ", "
        << ((info.kind == DataConnector::MessageInfo::Kind::Channel)
            ? std::to_string(unsigned(info.channelIdx))
            : "NULL") << ", "
        << "NULL, "
        << "NULL, "
        << ToSqlString(info.text) << ", "
        << status << ", "
        << "NULL, "
        << ToSqlNullableString(prefix6Hex) << ", "
        << (std::isnan(info.snrDb) ? "NULL" : std::to_string(info.snrDb)) << ", "
        << ((info.pathLen == 255) ? "NULL" : std::to_string(unsigned(info.pathLen)))
        << ")";

    if (!Execute(chatSql.str()))
    {
        std::cout << "[MeshDB] StoreMessage: inserted into chat_messages FAILED\n";
        return false;
    }

    std::cout << "[MeshDB] StoreMessage: inserted into chat_messages\n";

    return InsertEvent(
        static_cast<uint8_t>(DataConnector::EventType::Message),
        "message",
        summary,
        info.fromName,
        prefix6Hex,
        "",
        "",
        "",
        "",
        -1,
        -1,
        -1,
        1,
        0,
        0,
        nullptr);
}

bool MeshDB::StorePushAdvert(const DataConnector::PushAdvertInfo& info, const std::string& summary)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    if (!UpsertNodeFromPushAdvert(info))
    {
        return false;
    }

    const std::string prefix6Hex = info.valid
        ? DataConnector::hexBytes(info.prefix6.data(), info.prefix6.size())
        : "";

    return InsertEvent(
        static_cast<uint8_t>(DataConnector::EventType::PushAdvert),
        "push_advert",
        summary,
        info.valid ? info.name : "",
        prefix6Hex,
        "",
        "",
        "",
        "",
        static_cast<int>(info.payloadLen),
        -1,
        -1,
        info.valid ? 1 : 0,
        0,
        0,
        nullptr);
}

bool MeshDB::StorePushPathUpdated(const DataConnector::PushPathUpdatedInfo& info, const std::string& summary)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    if (!UpsertNodeFromPathUpdated(info))
    {
        return false;
    }

    const std::string publicKeyHex = info.valid
        ? DataConnector::hexBytes(info.publicKey.data(), info.publicKey.size())
        : "";

    return InsertEvent(
        static_cast<uint8_t>(DataConnector::EventType::PushPathUpdated),
        "push_path_updated",
        summary,
        "",
        "",
        publicKeyHex,
        "",
        "",
        "",
        static_cast<int>(info.payloadLen),
        -1,
        -1,
        info.valid ? 1 : 0,
        0,
        0,
        nullptr);
}

bool MeshDB::StorePushSendConfirmed(const DataConnector::PushSendConfirmedInfo& info, const std::string& summary)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    const std::string ackHex = info.valid
        ? DataConnector::u32hex(info.ack)
        : "";

    return InsertEvent(
        static_cast<uint8_t>(DataConnector::EventType::PushSendConfirmed),
        "push_send_confirmed",
        summary,
        "",
        "",
        "",
        ackHex,
        "",
        "",
        static_cast<int>(info.payloadLen),
        info.valid ? static_cast<int>(info.rttMs) : -1,
        -1,
        info.valid ? 1 : 0,
        0,
        0,
        nullptr);
}

bool MeshDB::StorePushSimple(const DataConnector::PushSimpleInfo& info, const std::string& summary)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    return InsertEvent(
        static_cast<uint8_t>(DataConnector::EventType::PushSimple),
        info.label,
        summary,
        "",
        "",
        "",
        "",
        "",
        "",
        static_cast<int>(info.payloadLen),
        -1,
        -1,
        1,
        0,
        0,
        nullptr);
}

bool MeshDB::StorePushTrace(const DataConnector::PushTraceInfo& info, const std::string& summary)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    unsigned long long eventId = 0;

    if (!InsertEvent(
            static_cast<uint8_t>(DataConnector::EventType::PushTrace),
            "push_trace",
            summary,
            "",
            "",
            "",
            "",
            info.valid ? DataConnector::u32hex(info.tag) : "",
            info.valid ? DataConnector::u32hex(info.authCode) : "",
            static_cast<int>(info.payloadLen),
            -1,
            info.valid ? static_cast<int>(info.flags) : -1,
            info.valid ? 1 : 0,
            0,
            0,
            &eventId))
    {
        return false;
    }

    if (!info.valid)
    {
        return true;
    }

    const size_t hopCount = info.pathHashes.size();

    for (size_t i = 0; i < hopCount; i++)
    {
        std::ostringstream hopSql;
        hopSql
            << "INSERT INTO event_trace_hops (event_id, hop_index, path_hash_hex, snr_db) VALUES ("
            << eventId << ", "
            << i << ", "
            << ToSqlString(DataConnector::hexBytes(&info.pathHashes[i], 1)) << ", ";

        if (i < info.snrDb.size())
        {
            hopSql << info.snrDb[i];
        }
        else
        {
            hopSql << "NULL";
        }

        hopSql << ")";

        if (!Execute(hopSql.str()))
        {
            return false;
        }
    }

    return true;
}

bool MeshDB::StorePushNewAdvert(const DataConnector::PushNewAdvertInfo& info, const std::string& summary)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    if (!UpsertNodeFromPushNewAdvert(info))
    {
        return false;
    }

    const std::string prefix6Hex = info.valid
        ? DataConnector::hexBytes(info.prefix6.data(), info.prefix6.size())
        : "";

    return InsertEvent(
        static_cast<uint8_t>(DataConnector::EventType::PushNewAdvert),
        "push_new_advert",
        summary,
        info.valid ? info.name : "",
        prefix6Hex,
        "",
        "",
        "",
        "",
        static_cast<int>(info.payloadLen),
        -1,
        -1,
        info.valid ? 1 : 0,
        info.valid ? static_cast<int>(info.advLatE6) : 0,
        info.valid ? static_cast<int>(info.advLonE6) : 0,
        nullptr);
}

bool MeshDB::StorePushUnknown(const DataConnector::PushUnknownInfo& info, const std::string& summary)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    return InsertEvent(
        static_cast<uint8_t>(DataConnector::EventType::PushUnknown),
        "push_unknown",
        summary,
        "",
        "",
        "",
        "",
        "",
        "",
        static_cast<int>(info.payloadLen),
        -1,
        static_cast<int>(info.code),
        1,
        0,
        0,
        nullptr);
}

std::string MeshDB::SanitizeUtf8(const std::string& value)
{
    std::string out;
    out.reserve(value.size());

    const unsigned char* s =
        reinterpret_cast<const unsigned char*>(value.data());

    size_t i = 0;
    const size_t n = value.size();

    while (i < n)
    {
        const unsigned char c = s[i];

        if (c <= 0x7F)
        {
            out.push_back(static_cast<char>(c));
            i++;
            continue;
        }

        if ((c & 0xE0) == 0xC0)
        {
            if ((i + 1 < n) &&
                ((s[i + 1] & 0xC0) == 0x80))
            {
                out.append(value, i, 2);
                i += 2;
                continue;
            }

            out.push_back('?');
            i++;
            continue;
        }

        if ((c & 0xF0) == 0xE0)
        {
            if ((i + 2 < n) &&
                ((s[i + 1] & 0xC0) == 0x80) &&
                ((s[i + 2] & 0xC0) == 0x80))
            {
                out.append(value, i, 3);
                i += 3;
                continue;
            }

            out.push_back('?');
            i++;
            continue;
        }

        if ((c & 0xF8) == 0xF0)
        {
            if ((i + 3 < n) &&
                ((s[i + 1] & 0xC0) == 0x80) &&
                ((s[i + 2] & 0xC0) == 0x80) &&
                ((s[i + 3] & 0xC0) == 0x80))
            {
                out.append(value, i, 4);
                i += 4;
                continue;
            }

            out.push_back('?');
            i++;
            continue;
        }

        out.push_back('?');
        i++;
    }

    return out;
}

std::optional<MeshDB::OutgoingTx> MeshDB::FetchNextQueuedTx()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return std::nullopt;
    }

    const char* sql =
        "SELECT "
        "    id, "
        "    COALESCE(tx_kind, 0), "
        "    COALESCE(target_name, ''), "
        "    COALESCE(target_node_id, 0), "
        "    COALESCE(room_name, ''), "
        "    COALESCE(room_node_id, 0), "
        "    COALESCE(channel_name, ''), "
        "    COALESCE(channel_idx, 0), "
        "    message_text, "
        "    retry_count, "
        "    max_retries, "
        "    COALESCE(sender_timestamp, 0), "
        "    COALESCE(current_ack_hex, ''), "
        "    COALESCE(suggested_timeout_ms, 0) "
        "FROM tx_outbox "
        "WHERE status = 0 "
        "  AND next_attempt_epoch <= UNIX_TIMESTAMP() "
        "ORDER BY created_at ASC "
        "LIMIT 1";
        
    if (mysql_query(s_conn, sql) != 0)
    {
        std::cerr << "MeshDB SQL error: " << mysql_error(s_conn) << "\n";
        return std::nullopt;
    }

    MYSQL_RES* res = mysql_store_result(s_conn);

    if (res == nullptr)
    {
        return std::nullopt;
    }

    MYSQL_ROW row = mysql_fetch_row(res);

    if (row == nullptr)
    {
        mysql_free_result(res);
        return std::nullopt;
    }

    MeshDB::OutgoingTx tx = BuildOutgoingTxFromRow(row);

    mysql_free_result(res);
    return tx;
}

std::vector<MeshDB::OutgoingTx> MeshDB::FetchTimedOutWaitingTx(unsigned int limit)
{
    std::vector<OutgoingTx> out;

    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return out;
    }

    std::ostringstream oss;

    oss
        << "SELECT "
        << "    id, "
        << "    COALESCE(tx_kind, 0), "
        << "    COALESCE(target_name, ''), "
        << "    COALESCE(target_node_id, 0), "
        << "    COALESCE(room_name, ''), "
        << "    COALESCE(room_node_id, 0), "
        << "    COALESCE(channel_name, ''), "
        << "    COALESCE(channel_idx, 0), "
        << "    message_text, "
        << "    retry_count, "
        << "    max_retries, "
        << "    COALESCE(sender_timestamp, 0), "
        << "    COALESCE(current_ack_hex, ''), "
        << "    COALESCE(suggested_timeout_ms, 0) "
        << "FROM tx_outbox "
        << "WHERE status = 1 "
        << "  AND timeout_epoch IS NOT NULL "
        << "  AND timeout_epoch <= UNIX_TIMESTAMP() "
        << "ORDER BY timeout_epoch ASC "
        << "LIMIT " << limit;

    if (mysql_query(s_conn, oss.str().c_str()) != 0)
    {
        std::cerr << "MeshDB SQL error: " << mysql_error(s_conn) << "\n";
        return out;
    }

    MYSQL_RES* res = mysql_store_result(s_conn);

    if (res == nullptr)
    {
        return out;
    }

    MYSQL_ROW row = nullptr;

    while ((row = mysql_fetch_row(res)) != nullptr)
    {
        MeshDB::OutgoingTx tx = BuildOutgoingTxFromRow(row);
        out.push_back(std::move(tx));
    }

    mysql_free_result(res);
    return out;
}

bool MeshDB::MarkTxDone(unsigned long long id)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    std::ostringstream oss;

    oss
        << "UPDATE tx_outbox SET "
        << "status = " << unsigned(TxStatus::Done) << ", "
        << "updated_at = CURRENT_TIMESTAMP(), "
        << "last_error = NULL "
        << "WHERE id = " << id;

    return Execute(oss.str());
}

bool MeshDB::MarkTxWaitingAck(
    unsigned long long id,
    uint32_t resolvedNodeId,
    uint32_t senderTimestamp,
    uint8_t retryCount,
    uint32_t ack,
    uint32_t suggestedTimeoutMs)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    const uint32_t nowEpoch = static_cast<uint32_t>(std::time(nullptr));
    const uint32_t timeoutEpoch = NextTimeoutEpoch(suggestedTimeoutMs);

    std::ostringstream oss;

    oss
        << "UPDATE tx_outbox SET "
        << "status = " << unsigned(TxStatus::WaitingAck) << ", "
        << "target_node_id = " << resolvedNodeId << ", "
        << "sender_timestamp = " << senderTimestamp << ", "
        << "retry_count = " << unsigned(retryCount) << ", "
        << "current_ack_hex = " << ToSqlString(DataConnector::u32hex(ack)) << ", "
        << "suggested_timeout_ms = " << suggestedTimeoutMs << ", "
        << "last_attempt_epoch = " << nowEpoch << ", "
        << "timeout_epoch = " << timeoutEpoch << ", "
        << "last_error = NULL "
        << "WHERE id = " << id;

    return Execute(oss.str());
}

bool MeshDB::RequeueTx(
    unsigned long long id,
    uint8_t newRetryCount,
    const std::string& errorText,
    uint32_t nextAttemptEpoch)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    std::ostringstream oss;

    oss
        << "UPDATE tx_outbox SET "
        << "status = " << unsigned(TxStatus::Queued) << ", "
        << "retry_count = " << unsigned(newRetryCount) << ", "
        << "current_ack_hex = NULL, "
        << "suggested_timeout_ms = NULL, "
        << "timeout_epoch = NULL, "
        << "next_attempt_epoch = " << nextAttemptEpoch << ", "
        << "last_error = " << ToSqlNullableString(errorText) << " "
        << "WHERE id = " << id;

    return Execute(oss.str());
}

bool MeshDB::MarkTxDeferred(
    unsigned long long id,
    const std::string& errorText,
    uint32_t nextAttemptEpoch)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    std::ostringstream oss;

    oss
        << "UPDATE tx_outbox SET "
        << "status = " << unsigned(TxStatus::Queued) << ", "
        << "next_attempt_epoch = " << nextAttemptEpoch << ", "
        << "last_error = " << ToSqlNullableString(errorText) << " "
        << "WHERE id = " << id;

    return Execute(oss.str());
}

bool MeshDB::MarkTxFailed(
    unsigned long long id,
    const std::string& errorText)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    std::ostringstream oss;

    oss
        << "UPDATE tx_outbox SET "
        << "status = " << unsigned(TxStatus::Failed) << ", "
        << "current_ack_hex = NULL, "
        << "suggested_timeout_ms = NULL, "
        << "timeout_epoch = NULL, "
        << "last_error = " << ToSqlNullableString(errorText) << " "
        << "WHERE id = " << id;

    if (!Execute(oss.str()))
    {
        return false;
    }

    std::ostringstream chatUpd;

    chatUpd
        << "UPDATE chat_messages SET "
        << "status = 2 "
        << "WHERE tx_outbox_id = " << id;

    Execute(chatUpd.str());

    return true;
}

bool MeshDB::DeleteTxByAck(
    uint32_t ack,
    uint32_t rttMs)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    const std::string ackHex = DataConnector::u32hex(ack);

    std::ostringstream upd;

    upd
        << "UPDATE tx_outbox SET "
        << "last_rtt_ms = " << rttMs << " "
        << "WHERE current_ack_hex = " << ToSqlString(ackHex);

    if (!Execute(upd.str()))
    {
        return false;
    }

    std::ostringstream chatUpd;

    chatUpd
        << "UPDATE chat_messages SET "
        << "status = 1 "
        << "WHERE tx_outbox_id = ("
        << "SELECT id FROM tx_outbox WHERE current_ack_hex = "
        << ToSqlString(ackHex)
        << " LIMIT 1)";

    Execute(chatUpd.str());

    std::ostringstream del;

    del
        << "DELETE FROM tx_outbox "
        << "WHERE current_ack_hex = " << ToSqlString(ackHex);

    return Execute(del.str());
}

bool MeshDB::ClearAllTables()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    if (!Execute("SET FOREIGN_KEY_CHECKS = 0"))
    {
        return false;
    }

    bool ok =
        Execute("TRUNCATE TABLE event_trace_hops") &&
        Execute("TRUNCATE TABLE discover_results") &&
        Execute("TRUNCATE TABLE discover_jobs") &&
        Execute("TRUNCATE TABLE events") &&
        Execute("TRUNCATE TABLE messages") &&
        Execute("TRUNCATE TABLE nodes") &&
        Execute("TRUNCATE TABLE channels") &&
        Execute("TRUNCATE TABLE tx_outbox");

    Execute("SET FOREIGN_KEY_CHECKS = 1");

    return ok;
}

bool MeshDB::ClearNodesTable()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    return Execute("TRUNCATE TABLE nodes");
}

bool MeshDB::ClearTxBoxTable()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    return Execute("TRUNCATE TABLE tx_outbox");
}

std::optional<uint32_t> MeshDB::FindRoomNodeIdByName(const std::string& roomName)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return std::nullopt;
    }

    std::ostringstream oss;
    oss
        << "SELECT node_id "
        << "FROM nodes "
        << "WHERE advert_type = 3 "
        << "AND name = " << ToSqlString(roomName) << " "
        << "LIMIT 1";

    if (mysql_query(s_conn, oss.str().c_str()) != 0)
    {
        return std::nullopt;
    }

    MYSQL_RES* res = mysql_store_result(s_conn);

    if (res == nullptr)
    {
        return std::nullopt;
    }

    MYSQL_ROW row = mysql_fetch_row(res);

    if (row == nullptr || row[0] == nullptr)
    {
        mysql_free_result(res);
        return std::nullopt;
    }

    const uint32_t nodeId = static_cast<uint32_t>(std::strtoul(row[0], nullptr, 10));

    mysql_free_result(res);
    return nodeId;
}

bool MeshDB::UpdateTxRoomNodeId(unsigned long long id, uint32_t roomNodeId)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    std::ostringstream oss;
    oss
        << "UPDATE tx_outbox "
        << "SET room_node_id = " << roomNodeId << " "
        << "WHERE id = " << id;

    return Execute(oss.str());
}

bool MeshDB::UpsertRoomPassword(
    uint32_t roomNodeId,
    const std::string& roomName,
    const std::string& password)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    std::ostringstream oss;
    oss
        << "INSERT INTO room_credentials ("
        << "room_node_id, room_name, password"
        << ") VALUES ("
        << roomNodeId << ", "
        << ToSqlNullableString(roomName) << ", "
        << ToSqlString(password)
        << ") ON DUPLICATE KEY UPDATE "
        << "room_name = VALUES(room_name), "
        << "password = VALUES(password), "
        << "updated_at = CURRENT_TIMESTAMP";

    return Execute(oss.str());
}

std::optional<std::string> MeshDB::FindRoomPassword(uint32_t roomNodeId)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return std::nullopt;
    }

    std::ostringstream oss;
    oss
        << "SELECT password "
        << "FROM room_credentials "
        << "WHERE room_node_id = " << roomNodeId << " "
        << "LIMIT 1";

    if (mysql_query(s_conn, oss.str().c_str()) != 0)
    {
        return std::nullopt;
    }

    MYSQL_RES* res = mysql_store_result(s_conn);

    if (res == nullptr)
    {
        return std::nullopt;
    }

    MYSQL_ROW row = mysql_fetch_row(res);

    if (row == nullptr || row[0] == nullptr)
    {
        mysql_free_result(res);
        return std::nullopt;
    }

    const std::string password = row[0];
    mysql_free_result(res);
    return password;
}

bool MeshDB::DeleteRoomPassword(uint32_t roomNodeId)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    std::ostringstream oss;
    oss
        << "DELETE FROM room_credentials "
        << "WHERE room_node_id = " << roomNodeId;

    return Execute(oss.str());
}

bool MeshDB::MarkChannelObservedUnlocked(uint8_t channelIdx)
{
    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    std::ostringstream sql;
    sql
        << "INSERT INTO channels ("
        << "channel_idx, name, is_observed, has_local_context, enabled, last_seen_at"
        << ") VALUES ("
        << unsigned(channelIdx) << ", "
        << ToSqlString("Channel " + std::to_string(unsigned(channelIdx))) << ", "
        << "1, 0, 0, NOW()) "
        << "ON DUPLICATE KEY UPDATE "
        << "is_observed = 1, "
        << "last_seen_at = NOW()";

    return Execute(sql.str());
}

bool MeshDB::MarkChannelObserved(uint8_t channelIdx)
{
    std::lock_guard<std::mutex> lock(s_mutex);
    return MarkChannelObservedUnlocked(channelIdx);
}

bool MeshDB::UpsertLocalChannel(
    uint8_t channelIdx,
    const std::string& name,
    uint8_t joinMode,
    const std::string& passphrase,
    const std::string& keyHex,
    bool enabled,
    bool isDefault)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    std::ostringstream sql;
    sql
        << "INSERT INTO channels ("
        << "channel_idx, name, join_mode, passphrase, key_hex, "
        << "enabled, is_default, is_observed, has_local_context, "
        << "sync_pending, sync_action, sync_error, "
        << "last_seen_at"
        << ") VALUES ("
        << unsigned(channelIdx) << ", "
        << ToSqlString(name) << ", "
        << unsigned(joinMode) << ", "
        << ToSqlNullableString(passphrase) << ", "
        << ToSqlNullableString(keyHex) << ", "
        << BoolToSql(enabled) << ", "
        << BoolToSql(isDefault) << ", "
        << "0, "
        << "1, "
        << "0, '', '', "
        << "NOW()"
        << ") ON DUPLICATE KEY UPDATE "
        << "name = VALUES(name), "
        << "join_mode = VALUES(join_mode), "
        << "passphrase = VALUES(passphrase), "
        << "key_hex = VALUES(key_hex), "
        << "enabled = VALUES(enabled), "
        << "is_default = VALUES(is_default), "
        << "has_local_context = 1, "
        << "sync_pending = 0, "
        << "sync_action = '', "
        << "sync_error = '', "
        << "last_seen_at = NOW()";

    return Execute(sql.str());
}

std::optional<MeshDB::ChannelRecord> MeshDB::FindChannelByIdx(uint8_t channelIdx)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return std::nullopt;
    }

    std::ostringstream sql;

    sql
        << "SELECT channel_idx, name, join_mode, passphrase, key_hex, "
        << "enabled, is_default, is_observed, has_local_context, "
        << "sync_pending, sync_action, sync_error, "
        << "UNIX_TIMESTAMP(last_seen_at) "
        << "FROM channels WHERE channel_idx = " << unsigned(channelIdx)
        << " LIMIT 1";
        
    if (mysql_query(s_conn, sql.str().c_str()) != 0)
    {
        std::cerr << "MeshDB SQL error: " << mysql_error(s_conn) << "\n";
        return std::nullopt;
    }

    MYSQL_RES* res = mysql_store_result(s_conn);

    if (res == nullptr)
    {
        return std::nullopt;
    }

    MYSQL_ROW row = mysql_fetch_row(res);

    if (row == nullptr)
    {
        mysql_free_result(res);
        return std::nullopt;
    }

    ChannelRecord rec {};
    rec.channelIdx = RowU8(row, 0);
    rec.name = RowString(row, 1);
    rec.joinMode = RowU8(row, 2);
    rec.passphrase = RowString(row, 3);
    rec.keyHex = RowString(row, 4);
    rec.enabled = RowU8(row, 5) != 0;
    rec.isDefault = RowU8(row, 6) != 0;
    rec.isObserved = RowU8(row, 7) != 0;
    rec.hasLocalContext = RowU8(row, 8) != 0;
    rec.syncPending = RowU8(row, 9) != 0;
    rec.syncAction = RowString(row, 10);
    rec.syncError = RowString(row, 11);
    rec.lastSeenEpoch = RowU32(row, 12);

    mysql_free_result(res);
    return rec;
}

std::vector<MeshDB::ChannelRecord> MeshDB::ListChannels(bool includeObserved)
{
    std::vector<ChannelRecord> out;

    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return out;
    }

    std::ostringstream sql;
    sql
        << "SELECT channel_idx, name, join_mode, passphrase, key_hex, "
        << "enabled, is_default, is_observed, has_local_context, "
        << "sync_pending, sync_action, sync_error, "
        << "UNIX_TIMESTAMP(last_seen_at) "
        << "FROM channels ";

    if (!includeObserved)
    {
        sql << "WHERE has_local_context = 1 ";
    }

    sql << "ORDER BY channel_idx ASC";

    if (mysql_query(s_conn, sql.str().c_str()) != 0)
    {
        std::cerr << "MeshDB SQL error: " << mysql_error(s_conn) << "\n";
        return out;
    }

    MYSQL_RES* res = mysql_store_result(s_conn);

    if (res == nullptr)
    {
        return out;
    }

    MYSQL_ROW row = nullptr;

    while ((row = mysql_fetch_row(res)) != nullptr)
    {
        ChannelRecord rec {};
        rec.channelIdx = RowU8(row, 0);
        rec.name = RowString(row, 1);
        rec.joinMode = RowU8(row, 2);
        rec.passphrase = RowString(row, 3);
        rec.keyHex = RowString(row, 4);
        rec.enabled = RowU8(row, 5) != 0;
        rec.isDefault = RowU8(row, 6) != 0;
        rec.isObserved = RowU8(row, 7) != 0;
        rec.hasLocalContext = RowU8(row, 8) != 0;
        rec.syncPending = RowU8(row, 9) != 0;
        rec.syncAction = RowString(row, 10);
        rec.syncError = RowString(row, 11);
        rec.lastSeenEpoch = RowU32(row, 12);

        out.push_back(std::move(rec));
    }

    mysql_free_result(res);
    return out;
}

bool MeshDB::DeleteChannel(uint8_t channelIdx)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    std::ostringstream sql1;
    sql1
        << "UPDATE channels SET "
        << "has_local_context = 0, "
        << "enabled = 0, "
        << "is_default = 0, "
        << "sync_pending = 0, "
        << "sync_action = '', "
        << "sync_error = '' "
        << "WHERE channel_idx = "
        << unsigned(channelIdx);

    if (!Execute(sql1.str()))
    {
        return false;
    }

    std::ostringstream sql2;
    sql2
        << "DELETE FROM channels "
        << "WHERE channel_idx = " << unsigned(channelIdx)
        << " AND has_local_context = 0"
        << " AND is_observed = 0";

    return Execute(sql2.str());
}

bool MeshDB::SaveCompanionConfig(
    const std::string& name,
    int32_t latitudeE6,
    int32_t longitudeE6,
    uint32_t radioBwHz,
    uint8_t radioSf,
    uint8_t radioCr)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    std::ostringstream sql;
    sql
        << "INSERT INTO companion_config ("
        << "id, name, latitude_e6, longitude_e6, radio_bw_hz, radio_sf, radio_cr, apply_pending, last_error"
        << ") VALUES ("
        << "1, "
        << ToSqlString(name) << ", "
        << latitudeE6 << ", "
        << longitudeE6 << ", "
        << radioBwHz << ", "
        << unsigned(radioSf) << ", "
        << unsigned(radioCr) << ", "
        << "1, "
        << "NULL"
        << ") "
        << "ON DUPLICATE KEY UPDATE "
        << "name = VALUES(name), "
        << "latitude_e6 = VALUES(latitude_e6), "
        << "longitude_e6 = VALUES(longitude_e6), "
        << "radio_bw_hz = VALUES(radio_bw_hz), "
        << "radio_sf = VALUES(radio_sf), "
        << "radio_cr = VALUES(radio_cr), "
        << "apply_pending = 1, "
        << "last_error = NULL";

    return Execute(sql.str());
}

std::optional<MeshDB::CompanionConfig> MeshDB::LoadCompanionConfig()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return std::nullopt;
    }

    const char* sql =
        "SELECT "
        "name, latitude_e6, longitude_e6, radio_bw_hz, radio_sf, radio_cr, apply_pending, last_error "
        "FROM companion_config "
        "WHERE id = 1 "
        "LIMIT 1";

    if (mysql_query(s_conn, sql) != 0)
    {
        std::cerr << "MeshDB SQL error: " << mysql_error(s_conn) << "\n";
        return std::nullopt;
    }

    MYSQL_RES* res = mysql_store_result(s_conn);

    if (res == nullptr)
    {
        return std::nullopt;
    }

    MYSQL_ROW row = mysql_fetch_row(res);

    if (row == nullptr)
    {
        mysql_free_result(res);
        return std::nullopt;
    }

    CompanionConfig cfg {};
    cfg.name = RowStr(row, 0);
    cfg.latitudeE6 = static_cast<int32_t>(RowI32(row, 1));
    cfg.longitudeE6 = static_cast<int32_t>(RowI32(row, 2));
    cfg.radioBwHz = RowU32(row, 3);
    cfg.radioSf = RowU8(row, 4);
    cfg.radioCr = RowU8(row, 5);
    cfg.applyPending = (RowU8(row, 6) != 0);
    cfg.lastError = RowNullableStr(row, 7).value_or("");

    mysql_free_result(res);
    return cfg;
}

bool MeshDB::MarkCompanionConfigApplied()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    const char* sql =
        "UPDATE companion_config "
        "SET apply_pending = 0, "
        "    last_applied_at = NOW(), "
        "    last_error = NULL "
        "WHERE id = 1";

    return Execute(sql);
}

bool MeshDB::MarkCompanionConfigApplyFailed(const std::string& error)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    std::ostringstream sql;
    sql
        << "UPDATE companion_config "
        << "SET apply_pending = 1, "
        << "    last_error = " << ToSqlString(error) << " "
        << "WHERE id = 1";

    return Execute(sql.str());
}

std::optional<std::string> MeshDB::FindNodeNameByPrefix4(
    const std::array<uint8_t, 4>& prefix4)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return std::nullopt;
    }

    const std::string prefix4Hex =
        DataConnector::hexBytes(prefix4.data(), prefix4.size());

    std::ostringstream oss;
    oss
        << "SELECT name "
        << "FROM nodes "
        << "WHERE public_key_hex LIKE " << ToSqlString(prefix4Hex + "%") << " "
        << "AND name <> '' "
        << "LIMIT 1";

    if (mysql_query(s_conn, oss.str().c_str()) != 0)
    {
        return std::nullopt;
    }

    MYSQL_RES* res = mysql_store_result(s_conn);

    if (res == nullptr)
    {
        return std::nullopt;
    }

    MYSQL_ROW row = mysql_fetch_row(res);

    if ((row == nullptr) || (row[0] == nullptr))
    {
        mysql_free_result(res);
        return std::nullopt;
    }

    const std::string name = row[0];

    mysql_free_result(res);
    return name;
}

static MeshDB::DiscoverJob BuildDiscoverJobFromRow(MYSQL_ROW row)
{
    MeshDB::DiscoverJob job {};

    job.id = row[0] ? std::strtoull(row[0], nullptr, 10) : 0;
    job.typeFilter = row[1] ? static_cast<uint8_t>(std::strtoul(row[1], nullptr, 10)) : 0;
    job.requestTag = row[2] ? static_cast<uint32_t>(std::strtoul(row[2], nullptr, 10)) : 0;
    job.resultCount = row[3] ? static_cast<uint32_t>(std::strtoul(row[3], nullptr, 10)) : 0;
    job.requestedBy = row[4] ? row[4] : "";
    job.errorText = row[5] ? row[5] : "";

    return job;
}

bool MeshDB::CreateDiscoverJob(
    uint8_t typeFilter,
    const std::string& requestedBy,
    unsigned long long* insertedId)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    std::ostringstream oss;

    oss
        << "INSERT INTO discover_jobs ("
        << "type_filter, requested_by, status"
        << ") VALUES ("
        << unsigned(typeFilter) << ", "
        << ToSqlNullableString(requestedBy) << ", "
        << unsigned(DiscoverStatus::Queued)
        << ")";

    if (!Execute(oss.str()))
    {
        return false;
    }

    if (insertedId != nullptr)
    {
        *insertedId = static_cast<unsigned long long>(mysql_insert_id(s_conn));
    }

    return true;
}

std::optional<MeshDB::DiscoverJob> MeshDB::FetchNextQueuedDiscoverJob()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return std::nullopt;
    }

    const char* sql =
        "SELECT "
        "    id, "
        "    COALESCE(type_filter, 0), "
        "    COALESCE(request_tag, 0), "
        "    COALESCE(result_count, 0), "
        "    COALESCE(requested_by, ''), "
        "    COALESCE(error_text, '') "
        "FROM discover_jobs "
        "WHERE status = 0 "
        "ORDER BY created_at ASC "
        "LIMIT 1";

    if (mysql_query(s_conn, sql) != 0)
    {
        std::cerr << "MeshDB SQL error: " << mysql_error(s_conn) << "\n";
        return std::nullopt;
    }

    MYSQL_RES* res = mysql_store_result(s_conn);

    if (res == nullptr)
    {
        return std::nullopt;
    }

    MYSQL_ROW row = mysql_fetch_row(res);

    if (row == nullptr)
    {
        mysql_free_result(res);
        return std::nullopt;
    }

    MeshDB::DiscoverJob job = BuildDiscoverJobFromRow(row);

    mysql_free_result(res);
    return job;
}

bool MeshDB::MarkDiscoverJobRunning(
    unsigned long long id,
    uint32_t requestTag)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    std::ostringstream oss;

    oss
        << "UPDATE discover_jobs SET "
        << "status = " << unsigned(DiscoverStatus::Running) << ", "
        << "request_tag = " << requestTag << ", "
        << "started_at = CURRENT_TIMESTAMP, "
        << "finished_at = NULL, "
        << "error_text = NULL "
        << "WHERE id = " << id;

    return Execute(oss.str());
}

bool MeshDB::InsertDiscoverResult(
    const DiscoverResultRow& row)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    std::ostringstream oss;

    oss
        << "INSERT INTO discover_results ("
        << "last_job_id, node_id_hex, snr_rx_db, snr_tx_db, rssi_dbm, source_code"
        << ") VALUES ("
        << row.lastJobId << ", "
        << ToSqlString(row.nodeIdHex) << ", "
        << row.snrRxDb << ", "
        << row.snrTxDb << ", "
        << row.rssiDbm << ", "
        << unsigned(row.sourceCode)
        << ") "
        << "ON DUPLICATE KEY UPDATE "
        << "last_job_id = VALUES(last_job_id), "
        << "snr_rx_db = VALUES(snr_rx_db), "
        << "snr_tx_db = VALUES(snr_tx_db), "
        << "rssi_dbm = VALUES(rssi_dbm), "
        << "source_code = VALUES(source_code)";

    return Execute(oss.str());
}

bool MeshDB::MarkDiscoverJobDone(
    unsigned long long id)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    std::ostringstream oss;

    oss
        << "UPDATE discover_jobs SET "
        << "status = " << unsigned(DiscoverStatus::Done) << ", "
        << "finished_at = CURRENT_TIMESTAMP, "
        << "error_text = NULL "
        << "WHERE id = " << id;

    return Execute(oss.str());
}

bool MeshDB::MarkDiscoverJobFailed(
    unsigned long long id,
    const std::string& errorText)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    std::ostringstream oss;

    oss
        << "UPDATE discover_jobs SET "
        << "status = " << unsigned(DiscoverStatus::Failed) << ", "
        << "finished_at = CURRENT_TIMESTAMP, "
        << "error_text = " << ToSqlNullableString(errorText) << " "
        << "WHERE id = " << id;

    return Execute(oss.str());
}

bool MeshDB::UpdateDiscoverJobResultCount(
    unsigned long long id,
    uint32_t resultCount)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    std::ostringstream oss;

    oss
        << "UPDATE discover_jobs SET "
        << "result_count = " << resultCount << " "
        << "WHERE id = " << id;

    return Execute(oss.str());
}

bool MeshDB::CleanupOldDiscoverJobs(
    unsigned keepLastN)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready)
    {
        return false;
    }

    if (keepLastN == 0)
    {
        keepLastN = 1;
    }

    std::ostringstream oss;

    oss
        << "DELETE FROM discover_jobs "
        << "WHERE id NOT IN ("
        << "    SELECT id FROM ("
        << "        SELECT id "
        << "        FROM discover_jobs "
        << "        ORDER BY id DESC "
        << "        LIMIT " << keepLastN
        << "    ) AS keep_rows"
        << ")";

    return Execute(oss.str());
}

bool MeshDB::HasRecentDiscoverJob(
    unsigned cooldownSeconds)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    std::ostringstream oss;

    oss
        << "SELECT id "
        << "FROM discover_jobs "
        << "WHERE status IN (1, 2, 3) "
        << "  AND COALESCE(started_at, created_at) >= "
        << "      (CURRENT_TIMESTAMP - INTERVAL " << cooldownSeconds << " SECOND) "
        << "ORDER BY id DESC "
        << "LIMIT 1";

    if (mysql_query(s_conn, oss.str().c_str()) != 0)
    {
        std::cerr << "MeshDB SQL error: " << mysql_error(s_conn) << "\n";
        return false;
    }

    MYSQL_RES* res = mysql_store_result(s_conn);

    if (res == nullptr)
    {
        return false;
    }

    MYSQL_ROW row = mysql_fetch_row(res);
    const bool found = (row != nullptr);

    mysql_free_result(res);
    return found;
}

std::optional<MeshDB::DiscoverJob> MeshDB::FetchLatestQueuedDiscoverJob()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return std::nullopt;
    }

    const char* sql =
        "SELECT "
        "    id, "
        "    COALESCE(type_filter, 0), "
        "    COALESCE(request_tag, 0), "
        "    COALESCE(result_count, 0), "
        "    COALESCE(requested_by, ''), "
        "    COALESCE(error_text, '') "
        "FROM discover_jobs "
        "WHERE status = 0 "
        "ORDER BY id DESC "
        "LIMIT 1";

    if (mysql_query(s_conn, sql) != 0)
    {
        std::cerr << "MeshDB SQL error: " << mysql_error(s_conn) << "\n";
        return std::nullopt;
    }

    MYSQL_RES* res = mysql_store_result(s_conn);

    if (res == nullptr)
    {
        return std::nullopt;
    }

    MYSQL_ROW row = mysql_fetch_row(res);

    if (row == nullptr)
    {
        mysql_free_result(res);
        return std::nullopt;
    }

    MeshDB::DiscoverJob job = BuildDiscoverJobFromRow(row);

    mysql_free_result(res);
    return job;
}

bool MeshDB::SkipOlderQueuedDiscoverJobs(
    unsigned long long keepJobId)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    std::ostringstream oss;

    oss
        << "UPDATE discover_jobs SET "
        << "status = " << unsigned(DiscoverStatus::Skipped) << ", "
        << "finished_at = CURRENT_TIMESTAMP, "
        << "error_text = " << ToSqlNullableString("superseded by newer discover job") << " "
        << "WHERE status = " << unsigned(DiscoverStatus::Queued) << " "
        << "AND id < " << keepJobId;

    return Execute(oss.str());
}

std::string MeshDB::ResolveChannelDisplayName(uint8_t channelIdx)
{
    if (auto rec = FindChannelByIdx(channelIdx); rec.has_value() && !rec->name.empty())
    {
        return rec->name;
    }

    if (channelIdx == 0)
    {
        return "Public";
    }

    return "Channel " + std::to_string(channelIdx);
}

std::vector<MeshDB::ChannelRecord> MeshDB::ListPendingChannelSync()
{
    std::vector<ChannelRecord> out;

    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return out;
    }

    const char* sql =
        "SELECT channel_idx, name, join_mode, passphrase, key_hex, "
        "enabled, is_default, is_observed, has_local_context, "
        "sync_pending, sync_action, sync_error, "
        "UNIX_TIMESTAMP(last_seen_at) "
        "FROM channels "
        "WHERE sync_pending = 1 "
        "ORDER BY channel_idx ASC";

    if (mysql_query(s_conn, sql) != 0)
    {
        std::cerr << "MeshDB SQL error: " << mysql_error(s_conn) << "\n";
        return out;
    }

    MYSQL_RES* res = mysql_store_result(s_conn);

    if (res == nullptr)
    {
        return out;
    }

    MYSQL_ROW row = nullptr;

    while ((row = mysql_fetch_row(res)) != nullptr)
    {
        ChannelRecord rec {};
        rec.channelIdx = RowU8(row, 0);
        rec.name = RowString(row, 1);
        rec.joinMode = RowU8(row, 2);
        rec.passphrase = RowString(row, 3);
        rec.keyHex = RowString(row, 4);
        rec.enabled = RowU8(row, 5) != 0;
        rec.isDefault = RowU8(row, 6) != 0;
        rec.isObserved = RowU8(row, 7) != 0;
        rec.hasLocalContext = RowU8(row, 8) != 0;
        rec.syncPending = RowU8(row, 9) != 0;
        rec.syncAction = RowString(row, 10);
        rec.syncError = RowString(row, 11);
        rec.lastSeenEpoch = RowU32(row, 12);

        out.push_back(std::move(rec));
    }

    mysql_free_result(res);
    return out;
}

bool MeshDB::MarkChannelSyncError(uint8_t channelIdx, const std::string& errorText)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    std::ostringstream sql;
    sql
        << "UPDATE channels SET "
        << "sync_pending = 0, "
        << "sync_action = '', "
        << "sync_error = " << ToSqlString(errorText) << " "
        << "WHERE channel_idx = " << unsigned(channelIdx)
        << " LIMIT 1";

    return Execute(sql.str());
}

bool MeshDB::MarkChannelDeletePending(uint8_t channelIdx)
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    std::ostringstream sql;
    sql
        << "UPDATE channels SET "
        << "sync_pending = 1, "
        << "sync_action = 'delete', "
        << "sync_error = '' "
        << "WHERE channel_idx = " << unsigned(channelIdx)
        << " LIMIT 1";

    return Execute(sql.str());
}

bool MeshDB::ClearChannelsTable()
{
    std::lock_guard<std::mutex> lock(s_mutex);

    if (!s_ready || (s_conn == nullptr))
    {
        return false;
    }

    return Execute("DELETE FROM channels");
}
