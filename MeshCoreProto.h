#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

// Wire-format / protocol helpers for MeshCore companion-radio protocol.
// This module intentionally contains:
//  - numeric command/response/push codes
//  - endian helpers
//  - frame decoders (no I/O, no threads)
// So higher layers (Client, app) stay clean.

namespace MeshCoreProto
{
    // Commands
    static constexpr uint8_t CMD_APP_START         = 1;
    static constexpr uint8_t CMD_SEND_TXT_MSG      = 2;
    static constexpr uint8_t CMD_SEND_CHANNEL_TXT_MSG = 3;
    static constexpr uint8_t CMD_GET_CONTACTS      = 4;
    static constexpr uint8_t CMD_GET_DEVICE_TIME   = 5;
    static constexpr uint8_t CMD_SET_DEVICE_TIME   = 6;
    static constexpr uint8_t CMD_SEND_SELF_ADVERT  = 7;
    static constexpr uint8_t CMD_SET_ADVERT_NAME = 8;
    static constexpr uint8_t CMD_ADD_UPDATE_CONTACT = 9;
    static constexpr uint8_t CMD_SYNC_NEXT_MESSAGE = 10;
    static constexpr uint8_t CMD_SET_RADIO_PARAMS  = 11;
    static constexpr uint8_t CMD_SET_RADIO_TX_POWER = 12;
    static constexpr uint8_t CMD_RESET_PATH = 13;
    static constexpr uint8_t CMD_SET_ADVERT_LATLON = 14;
    static constexpr uint8_t CMD_REMOVE_CONTACT = 15;
    static constexpr uint8_t CMD_DEVICE_QUERY      = 22;
    static constexpr uint8_t CMD_SEND_LOGIN        = 26;
    static constexpr uint8_t CMD_SET_OTHER_PARAMS  = 38;
    static constexpr uint8_t CMD_SEND_RAW_DATA      = 0x32;
    static constexpr uint8_t CMD_GET_CHANNEL_INFO   = 0x1F;
    static constexpr uint8_t CMD_SET_CHANNEL        = 0x20;
    static constexpr uint8_t CMD_SEND_CONTROL_DATA = 0x37;
    static constexpr uint8_t CMD_GET_STATS_RADIO   = 0x38;
    static constexpr uint8_t CONTROL_OP_DISCOVER = 0x81;
    static constexpr uint8_t CMD_SET_PATH_HASH_MODE = 0x3D;

    // Responses
    static constexpr uint8_t RESP_CODE_OK                  = 0;
    static constexpr uint8_t RESP_CODE_ERR                 = 1;
    static constexpr uint8_t RESP_CODE_CONTACTS_START      = 2;
    static constexpr uint8_t RESP_CODE_CONTACT             = 3;
    static constexpr uint8_t RESP_CODE_END_OF_CONTACTS     = 4;
    static constexpr uint8_t RESP_CODE_SELF_INFO           = 5;
    static constexpr uint8_t RESP_CODE_SENT                = 6;
    static constexpr uint8_t RESP_CODE_CONTACT_MSG_RECV    = 7;
    static constexpr uint8_t RESP_CODE_CHANNEL_MSG_RECV    = 8;
    static constexpr uint8_t RESP_CODE_CURR_TIME           = 9;
    static constexpr uint8_t RESP_CODE_NO_MORE_MESSAGES    = 10;
    static constexpr uint8_t RESP_CODE_DEVICE_INFO         = 13;
    static constexpr uint8_t RESP_CODE_CONTACT_MSG_RECV_V3 = 16;
    static constexpr uint8_t RESP_CODE_CHANNEL_MSG_RECV_V3 = 17;
    static constexpr uint8_t RESP_CODE_CHANNEL_INFO = 0x12;
    static constexpr uint8_t RESP_CODE_STATS_RADIO   = 0x18;

    // Discover Responses
    static constexpr uint8_t DISCOVER_FILTER_CHAT = 0x01;
    static constexpr uint8_t DISCOVER_FILTER_ROOM = 0x02;
    static constexpr uint8_t DISCOVER_FILTER_REPEATER = 0x04;

    // Push notifications
    static constexpr uint8_t PUSH_CODE_ADVERT             = 0x80;
    static constexpr uint8_t PUSH_CODE_PATH_UPDATED       = 0x81;
    static constexpr uint8_t PUSH_CODE_SEND_CONFIRMED     = 0x82;
    static constexpr uint8_t PUSH_CODE_MSG_WAITING        = 0x83;
    static constexpr uint8_t PUSH_CODE_RAW_DATA           = 0x84;
    static constexpr uint8_t PUSH_CODE_LOGIN_SUCCESS      = 0x85;
    static constexpr uint8_t PUSH_CODE_LOGIN_FAIL         = 0x86;
    static constexpr uint8_t PUSH_CODE_STATUS_RESPONSE    = 0x87;
    static constexpr uint8_t PUSH_CODE_RX_LOG_DATA        = 0x88;
    static constexpr uint8_t PUSH_CODE_TRACE_DATA         = 0x89;
    static constexpr uint8_t PUSH_CODE_NEW_ADVERT         = 0x8A;
    static constexpr uint8_t PUSH_CODE_TELEMETRY_RESPONSE = 0x8B;
    static constexpr uint8_t PUSH_CODE_BINARY_RESPONSE    = 0x8C;
    static constexpr uint8_t PUSH_CODE_CONTROL_DATA       = 0x8E;

    // Payload Types
    static constexpr uint8_t PAYLOAD_TYPE_REQ         = 0x00;
    static constexpr uint8_t PAYLOAD_TYPE_RESPONSE    = 0x01;
    static constexpr uint8_t PAYLOAD_TYPE_TXT_MSG     = 0x02;
    static constexpr uint8_t PAYLOAD_TYPE_ACK         = 0x03;
    static constexpr uint8_t PAYLOAD_TYPE_ADVERT      = 0x04;
    static constexpr uint8_t PAYLOAD_TYPE_GRP_TXT     = 0x05;
    static constexpr uint8_t PAYLOAD_TYPE_GRP_DATA    = 0x06;
    static constexpr uint8_t PAYLOAD_TYPE_ANON_REQ    = 0x07;
    static constexpr uint8_t PAYLOAD_TYPE_PATH        = 0x08;
    static constexpr uint8_t PAYLOAD_TYPE_TRACE       = 0x09;
    static constexpr uint8_t PAYLOAD_TYPE_MULTIPART   = 0x0A;
    static constexpr uint8_t PAYLOAD_TYPE_CONTROL     = 0x0B;
    static constexpr uint8_t PAYLOAD_TYPE_RESERVED_C  = 0x0C;
    static constexpr uint8_t PAYLOAD_TYPE_RESERVED_D  = 0x0D;
    static constexpr uint8_t PAYLOAD_TYPE_RESERVED_E  = 0x0E;
    static constexpr uint8_t PAYLOAD_TYPE_RAW_CUSTOM  = 0x0F;

    #include <cstdio>

    static const char* payloadTypeToString(uint8_t type)
    {
        static char buffer[32];

        const char* name;

        switch (type)
        {
            case PAYLOAD_TYPE_REQ:         name = "REQ"; break;
            case PAYLOAD_TYPE_RESPONSE:    name = "RESPONSE"; break;
            case PAYLOAD_TYPE_TXT_MSG:     name = "TXT_MSG"; break;
            case PAYLOAD_TYPE_ACK:         name = "ACK"; break;
            case PAYLOAD_TYPE_ADVERT:      name = "ADVERT"; break;
            case PAYLOAD_TYPE_GRP_TXT:     name = "GRP_TXT"; break;
            case PAYLOAD_TYPE_GRP_DATA:    name = "GRP_DATA"; break;
            case PAYLOAD_TYPE_ANON_REQ:    name = "ANON_REQ"; break;
            case PAYLOAD_TYPE_PATH:        name = "PATH"; break;
            case PAYLOAD_TYPE_TRACE:       name = "TRACE"; break;
            case PAYLOAD_TYPE_MULTIPART:   name = "MULTIPART"; break;
            case PAYLOAD_TYPE_CONTROL:     name = "CONTROL"; break;
            case PAYLOAD_TYPE_RESERVED_C:  name = "RESERVED_C"; break;
            case PAYLOAD_TYPE_RESERVED_D:  name = "RESERVED_D"; break;
            case PAYLOAD_TYPE_RESERVED_E:  name = "RESERVED_E"; break;
            case PAYLOAD_TYPE_RAW_CUSTOM:  name = "RAW_CUSTOM"; break;
            default:                       name = "UNKNOWN"; break;
        }

        std::snprintf(buffer, sizeof(buffer), "0x%02X (%s)", type, name);
        return buffer;
    }

    // Endian helpers (wire is little-endian unless noted).
    uint32_t le32(const uint8_t *p);
    uint32_t be32(const uint8_t *p);

    // Common decoded structures.
    struct ContactRecord
    {
        std::array<uint8_t, 32> publicKey {};
        uint8_t type {};
        uint8_t flags {};

        uint8_t unknown1 {};
        std::array<uint8_t, 64> unknown64 {};

        std::string name;
        uint32_t lastAdvert {}; // epoch secs (UTC)

        int32_t advLatE6 {};
        int32_t advLonE6 {};

        uint32_t lastMod {};    // epoch secs (UTC)

        uint32_t nodeId() const;
        std::array<uint8_t, 6> prefix6() const;

        double advLatDeg() const
        {
            return static_cast<double>(advLatE6) / 1000000.0;
        }

        double advLonDeg() const
        {
            return static_cast<double>(advLonE6) / 1000000.0;
        }

        bool hasLocation() const
        {
            return advLatE6 != 0 || advLonE6 != 0;
        }
    };

    struct TraceData
    {
        uint8_t flags;
        uint32_t tag;
        uint32_t authCode;
        std::vector<uint8_t> pathHashes; // length = pathLen
        std::vector<float> snrDb;        // length = pathLen+1 (byte/4.0)
    };

    struct LoginSuccessInfo
    {
        uint8_t permissions = 0;
        std::array<uint8_t, 6> prefix6 {};
        uint32_t tag = 0;
        uint8_t newPermissions = 0;
        bool hasNewPermissions = false;
    };

    struct DiscoverNode
    {
        bool valid = false;
        uint8_t sourceCode = 0;
        float snrRxDb = 0.0f;
        float snrTxDb = 0.0f;
        int rssiDbm = 0;
        uint32_t tag = 0;
        std::array<uint8_t, 8> nodeId {};
    };

    bool decodeDiscoverResponse(const std::vector<uint8_t>& frame, DiscoverNode& out);

    bool decodeLoginSuccess(const std::vector<uint8_t>& frame, LoginSuccessInfo& out);
    bool decodeLoginSuccessPayload(const std::vector<uint8_t>& payload, LoginSuccessInfo& out);

    // Decoders expect the full frame including code byte at [0].
    bool decodeContactRecord(const std::vector<uint8_t> &frame, ContactRecord &out);
    bool decodePublicKey32(const std::vector<uint8_t> &frame, std::array<uint8_t, 32> &outPk);
    bool decodeTraceData(const std::vector<uint8_t> &frame, TraceData &out);
}
