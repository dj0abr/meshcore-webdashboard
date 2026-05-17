#include "PushRouter.h"

#include "DataConnector.h"
#include "MeshCoreProto.h"
#include "MeshDB.h"
#include "MeshRxLogDecoder.h"
#include "MeshcoreMonitor.h"
#include "MessageCorrelation.h"

#include <array>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <openssl/sha.h>
#include <algorithm>
#include <cctype>

PushRouter::PushRouter(MeshCoreClient& client, AppRuntime& runtime)
    : m_client(client)
    , m_runtime(runtime)
{
}

namespace
{
    bool IsMonitorLoggedInHandler(uint8_t code)
    {
        return code == MeshCoreProto::PUSH_CODE_RX_LOG_DATA;
    }

    bool IsKnownPushCode(uint8_t code)
    {
        switch (code)
        {
            case MeshCoreProto::PUSH_CODE_ADVERT:
            case MeshCoreProto::PUSH_CODE_PATH_UPDATED:
            case MeshCoreProto::PUSH_CODE_SEND_CONFIRMED:
            case MeshCoreProto::PUSH_CODE_MSG_WAITING:
            case MeshCoreProto::PUSH_CODE_RAW_DATA:
            case MeshCoreProto::PUSH_CODE_LOGIN_SUCCESS:
            case MeshCoreProto::PUSH_CODE_LOGIN_FAIL:
            case MeshCoreProto::PUSH_CODE_STATUS_RESPONSE:
            case MeshCoreProto::PUSH_CODE_RX_LOG_DATA:
            case MeshCoreProto::PUSH_CODE_TRACE_DATA:
            case MeshCoreProto::PUSH_CODE_NEW_ADVERT:
            case MeshCoreProto::PUSH_CODE_TELEMETRY_RESPONSE:
            case MeshCoreProto::PUSH_CODE_BINARY_RESPONSE:
            case MeshCoreProto::PUSH_CODE_CONTROL_DATA:
                return true;

            default:
                return false;
        }
    }
}

void PushRouter::Attach()
{
    m_client.setPushCallback(
        [this](uint8_t code, const std::vector<uint8_t>& payload)
        {
            HandlePush(code, payload);
        });
}

void PushRouter::HandlePush(uint8_t code, const std::vector<uint8_t>& payload)
{
    //if(code >= 0x80) printf("********************************* [HandlePush] code: %02X\n",code);

    if (IsKnownPushCode(code) && !IsMonitorLoggedInHandler(code))
    {
        MeshcoreMonitor("PUSH", code, payload, nullptr);
    }

    switch (code)
    {
        case MeshCoreProto::PUSH_CODE_ADVERT:
        {
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            std::cout << "[DEBUG " << ms << "] PUSH_CODE_ADVERT" << std::endl;

            HandleAdvert(payload);
            return;
        }

        case MeshCoreProto::PUSH_CODE_PATH_UPDATED:
        {
            HandlePathUpdated(payload);
            return;
        }

        case MeshCoreProto::PUSH_CODE_SEND_CONFIRMED:
        {
            HandleSendConfirmed(payload);
            return;
        }

        case MeshCoreProto::PUSH_CODE_MSG_WAITING:
        {
            HandleMsgWaiting(payload);
            return;
        }

        case MeshCoreProto::PUSH_CODE_LOGIN_SUCCESS:
        {
            HandleLoginSuccess(payload);
            return;
        }

        case MeshCoreProto::PUSH_CODE_LOGIN_FAIL:
        {
            HandleLoginFail(payload);
            return;
        }

        case MeshCoreProto::PUSH_CODE_RX_LOG_DATA:
        {
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            std::cout << "[DEBUG " << ms << "] PUSH_CODE_RX_LOG_DATA size=" << payload.size() << std::endl;
            HandleLogRxData(payload);
            return;
        }

        case MeshCoreProto::PUSH_CODE_TRACE_DATA:
        {
            auto now = std::chrono::system_clock::now();
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
            std::cout << "[DEBUG " << ms << "] PUSH_CODE_TRACE_DATA size=" << payload.size() << std::endl;
            HandleTraceData(payload);
            return;
        }

        case MeshCoreProto::PUSH_CODE_NEW_ADVERT:
        {
            HandleNewAdvert(payload);
            return;
        }

        default:
        {
            HandleUnknown(code, payload);
            return;
        }
    }
}

void PushRouter::HandleAdvert(const std::vector<uint8_t>& payload)
{
    std::cout << "[DEBUG] Advert RAW (" << payload.size() << " bytes): ";
    for (auto b : payload)
    {
        printf("%02X ", b);
    }
    printf("\n");

    DataConnector::PushAdvertInfo info {};
    info.payloadLen = payload.size();

    std::array<uint8_t, 32> pk {};
    info.valid = MeshCoreProto::decodePublicKey32(payload, pk);

    if (info.valid)
    {
        std::cout << "[DEBUG] PublicKey decoded: ";

        for (auto b : pk)
        {
            printf("%02X", b);
        }
        printf("\n");
    }
    else
    {
        std::cout << "[DEBUG] PublicKey decode FAILED" << std::endl;
    }

    // Test: ist das vielleicht doch TraceData?
    MeshCoreProto::TraceData trace;
    if (MeshCoreProto::decodeTraceData(payload, trace))
    {
        std::cout << "[DEBUG] Advert enthält TRACE_DATA!" << std::endl;
    }

    if (info.valid)
    {
        for (size_t i = 0; i < 6; i++)
        {
            info.prefix6[i] = pk[i];
        }

        info.name = m_client.nameFromPrefix6(info.prefix6);
    }

    DataConnector::Emit(info);
    m_runtime.RequestContactSync();
}

void PushRouter::HandlePathUpdated(const std::vector<uint8_t>& payload)
{
    DataConnector::PushPathUpdatedInfo info {};
    info.payloadLen = payload.size();
    info.valid = MeshCoreProto::decodePublicKey32(payload, info.publicKey);

    DataConnector::Emit(info);
}

void PushRouter::HandleSendConfirmed(const std::vector<uint8_t>& payload)
{
    DataConnector::PushSendConfirmedInfo info {};
    info.payloadLen = payload.size();
    info.valid = (payload.size() >= 1 + 4 + 4);

    if (info.valid)
    {
        info.ack = MeshCoreProto::le32(payload.data() + 1);
        info.rttMs = MeshCoreProto::le32(payload.data() + 5);
    }

    DataConnector::Emit(info);

    if (info.valid)
    {
        MeshDB::DeleteTxByAck(info.ack, info.rttMs);
    }
}

void PushRouter::HandleMsgWaiting(const std::vector<uint8_t>& payload)
{
    (void)payload;

    DataConnector::PushSimpleInfo info
    {
        "PUSH MSG_WAITING (auto-sync)",
        0
    };

    DataConnector::Emit(info);
}

void PushRouter::HandleLoginSuccess(const std::vector<uint8_t>& payload)
{
    DataConnector::PushSimpleInfo info
    {
        "PUSH LOGIN_SUCCESS",
        payload.size()
    };

    DataConnector::Emit(info);

    MeshCoreProto::LoginSuccessInfo loginInfo {};

    if (!MeshCoreProto::decodeLoginSuccessPayload(payload, loginInfo))
    {
        std::cout << "[ROOM] LOGIN_SUCCESS decode failed\n";
        return;
    }

    m_runtime.NotifyRoomLoginSuccess(loginInfo.prefix6);
}

void PushRouter::HandleLoginFail(const std::vector<uint8_t>& payload)
{
    DataConnector::PushSimpleInfo info
    {
        "PUSH LOGIN_FAIL",
        payload.size()
    };

    DataConnector::Emit(info);
    m_runtime.NotifyRoomLoginFail();
}

// Schalte log ein oder aus
static bool g_rxLogDebug = false;
#define RXDBG(x) do { if (g_rxLogDebug) { x; } } while (0)


void PushRouter::HandleLogRxData(const std::vector<uint8_t>& payload)
{
    RXDBG(std::cout << "[DEBUG RX_LOG] HandleLogRxData called" << std::endl);
    RXDBG(std::cout << "[DEBUG RX_LOG] payload.size=" << payload.size() << std::endl);

    RXDBG(
        std::cout << "[DEBUG RX_LOG] RAW: ";
        for (uint8_t b : payload)
        {
            printf("%02X ", b);
        }
        printf("\n");
    );

    const MeshRxLogDecoder::DecodedPacket pkt =
        MeshRxLogDecoder::Decode(payload);

    MeshcoreMonitor(
        "PUSH RX_LOG",
        MeshCoreProto::PUSH_CODE_RX_LOG_DATA,
        payload,
        &pkt
    );

    RXDBG(std::cout << "[DEBUG RX_LOG] Decode result:" << std::endl);
    RXDBG(std::cout << "  valid=" << pkt.valid << std::endl);
    RXDBG(std::cout << "  pushCode=" << static_cast<int>(pkt.pushCode) << std::endl);
    RXDBG(std::cout << "  routeType=" << static_cast<int>(pkt.routeType) << std::endl);
    RXDBG(std::cout << "  payloadType=" << MeshCoreProto::payloadTypeToString(pkt.payloadType) << std::endl);
    RXDBG(std::cout << "  payloadVersion=" << static_cast<int>(pkt.payloadVersion) << std::endl);

    RXDBG(std::cout << "  snrDb=" << pkt.snrDb << std::endl);
    RXDBG(std::cout << "  rssiDbm=" << pkt.rssiDbm << std::endl);

    RXDBG(std::cout << "  pathLen=" << static_cast<int>(pkt.pathLen) << std::endl);
    RXDBG(std::cout << "  pathHashSize=" << static_cast<int>(pkt.pathHashSize) << std::endl);
    RXDBG(std::cout << "  pathText=" << MeshRxLogDecoder::FormatPath(pkt) << std::endl);

    RXDBG(
        std::cout
            << "  pktHash=0x"
            << std::hex
            << std::uppercase
            << std::setw(8)
            << std::setfill('0')
            << pkt.pktHash
            << std::dec
            << std::endl;
    );

    RXDBG(std::cout << "  originalHex.size=" << pkt.originalHex.size() << std::endl);

    RXDBG(std::cout << "  grpTxtValid=" << pkt.grpTxtValid << std::endl);
    RXDBG(std::cout << "  grpResolvedChannelName=" << pkt.grpResolvedChannelName << std::endl);
    RXDBG(std::cout << "  grpTimestamp=" << pkt.grpTimestamp << std::endl);
    RXDBG(std::cout << "  grpTxtType=" << static_cast<int>(pkt.grpTxtType) << std::endl);
    RXDBG(std::cout << "  grpText=" << pkt.grpText << std::endl);

    RXDBG(std::cout << "  advertValid=" << pkt.advertValid << std::endl);
    RXDBG(std::cout << "  advertPublicKey.size=" << pkt.advertPublicKey.size() << std::endl);

    if (!pkt.advertPublicKey.empty())
    {
        RXDBG(
            std::cout << "  advertPublicKey="
                      << MeshRxLogDecoder::BytesToHex(pkt.advertPublicKey)
                      << std::endl;
        );
    }

    RXDBG(std::cout << "  advertTimestamp=" << pkt.advertTimestamp << std::endl);
    RXDBG(std::cout << "  advertRole=" << static_cast<int>(pkt.advertRole) << std::endl);
    RXDBG(std::cout << "  advertHasGps=" << pkt.advertHasGps << std::endl);
    RXDBG(std::cout << "  advertHasBle=" << pkt.advertHasBle << std::endl);
    RXDBG(std::cout << "  advertHasShortcut=" << pkt.advertHasShortcut << std::endl);
    RXDBG(std::cout << "  advertHasName=" << pkt.advertHasName << std::endl);
    RXDBG(std::cout << "  advertLocationValid=" << pkt.advertLocationValid << std::endl);
    RXDBG(std::cout << "  advertLatitudeE6=" << pkt.advertLatitudeE6 << std::endl);
    RXDBG(std::cout << "  advertLongitudeE6=" << pkt.advertLongitudeE6 << std::endl);
    RXDBG(std::cout << "  advertName=" << pkt.advertName << std::endl);

    if (pkt.payloadType != MeshCoreProto::PAYLOAD_TYPE_GRP_TXT &&
        pkt.payloadType != MeshCoreProto::PAYLOAD_TYPE_ADVERT &&
        pkt.payloadType != MeshCoreProto::PAYLOAD_TYPE_TXT_MSG)
    {
        std::cout << "[DEBUG RX_LOG] DROP: unsupported payloadType"
                << std::endl;
        return;
    }

    DataConnector::PushRxLogInfo info {};
    info.valid = pkt.valid;
    info.payloadLen = payload.size();

    if (!pkt.valid)
    {
        std::cout << "[DEBUG RX_LOG] DROP/EMIT invalid packet" << std::endl; // Fehler → immer sichtbar
        DataConnector::Emit(info);
        return;
    }

    info.pushCode = pkt.pushCode;
    info.routeType = pkt.routeType;
    info.payloadType = pkt.payloadType;
    info.payloadVersion = pkt.payloadVersion;

    info.snrDb = pkt.snrDb;
    info.hasSnrDb = true;

    info.rssiDbm = pkt.rssiDbm;
    info.hasRssiDbm = true;

    info.pathLen = pkt.pathLen;
    info.hasPathLen = true;

    info.pathHashSize = pkt.pathHashSize;
    info.hasPathHashSize = true;

    info.pktHash = pkt.pktHash;
    info.hasPktHash = true;

    info.rawHex = pkt.originalHex;
    info.pathText = MeshRxLogDecoder::FormatPath(pkt);

    RXDBG(std::cout << "[DEBUG RX_LOG] info prepared:" << std::endl);
    RXDBG(std::cout << "  info.pathLen=" << info.pathLen << std::endl);
    RXDBG(std::cout << "  info.pathHashSize=" << info.pathHashSize << std::endl);
    RXDBG(std::cout << "  info.pathText=" << info.pathText << std::endl);
    RXDBG(std::cout << "  info.rawHex.size=" << info.rawHex.size() << std::endl);

    if (pkt.grpTxtValid)
    {
        RXDBG(std::cout << "[DEBUG RX_LOG] Processing group text" << std::endl);

        if (!pkt.grpResolvedChannelName.empty())
        {
            RXDBG(std::cout << "  lookup channel name=" << pkt.grpResolvedChannelName << std::endl);

            const auto channelRec = MeshDB::FindChannelByName(pkt.grpResolvedChannelName);

            if (channelRec.has_value())
            {
                info.channelIdx = channelRec->channelIdx;
                info.hasChannelIdx = true;

                RXDBG(std::cout << "  channelIdx=" << info.channelIdx << std::endl);
            }
            else
            {
                std::cout << "  channel lookup failed" << std::endl; // Fehler → immer sichtbar
            }
        }

        info.senderTimestamp = pkt.grpTimestamp;
        info.hasSenderTimestamp = true;

        info.txtType = pkt.grpTxtType;
        info.hasTxtType = true;

        info.messageText = pkt.grpText;
        info.hasMessageText = true;

        if (info.hasChannelIdx && info.hasSenderTimestamp && info.hasTxtType && info.hasMessageText)
        {
            info.correlationKey = MessageCorrelation::BuildKey(
                info.channelIdx,
                info.senderTimestamp,
                info.txtType,
                info.messageText);

            RXDBG(std::cout << "  correlationKey=" << info.correlationKey << std::endl);
        }
    }

    if (pkt.advertValid)
    {
        RXDBG(std::cout << "[DEBUG RX_LOG] Processing advert inside RX_LOG" << std::endl);

        info.hasAdvert = true;
        info.advertValid = true;

        if (!pkt.advertPublicKey.empty())
        {
            std::string pk = MeshRxLogDecoder::BytesToHex(pkt.advertPublicKey);

            std::transform(
                pk.begin(),
                pk.end(),
                pk.begin(),
                [](unsigned char c)
                {
                    return std::toupper(c);
                }
            );

            info.advertPublicKey = pk;
            info.hasAdvertPublicKey = true;

            RXDBG(std::cout << "  info.advertPublicKey=" << info.advertPublicKey << std::endl);
        }

        info.advertTimestamp = pkt.advertTimestamp;
        info.hasAdvertTimestamp = true;

        info.advertRole = pkt.advertRole;
        info.hasAdvertRole = true;

        info.advertHasGps = pkt.advertHasGps;
        info.hasAdvertHasGps = true;

        info.advertHasBle = pkt.advertHasBle;
        info.hasAdvertHasBle = true;

        info.advertHasShortcut = pkt.advertHasShortcut;
        info.hasAdvertHasShortcut = true;

        info.advertHasName = pkt.advertHasName;
        info.hasAdvertHasName = true;

        if (pkt.advertLocationValid)
        {
            info.advertLatitudeE6 = pkt.advertLatitudeE6;
            info.hasAdvertLatitudeE6 = true;

            info.advertLongitudeE6 = pkt.advertLongitudeE6;
            info.hasAdvertLongitudeE6 = true;

            RXDBG(std::cout << "  info.advertLatitudeE6=" << info.advertLatitudeE6 << std::endl);
            RXDBG(std::cout << "  info.advertLongitudeE6=" << info.advertLongitudeE6 << std::endl);
        }

        if (pkt.advertHasName && !pkt.advertName.empty())
        {
            info.advertName = pkt.advertName;
            info.hasAdvertName = true;

            RXDBG(std::cout << "  info.advertName=" << info.advertName << std::endl);
        }
    }

    RXDBG(std::cout << "[DEBUG RX_LOG] Emit(info)" << std::endl);
    DataConnector::Emit(info);
}

void PushRouter::HandleTraceData(const std::vector<uint8_t>& payload)
{
    DataConnector::PushTraceInfo info {};
    info.payloadLen = payload.size();

    MeshCoreProto::TraceData tr {};
    info.valid = MeshCoreProto::decodeTraceData(payload, tr);

    if (info.valid)
    {
        info.flags = tr.flags;
        info.tag = tr.tag;
        info.authCode = tr.authCode;
        info.pathHashes.assign(tr.pathHashes.begin(), tr.pathHashes.end());
        info.snrDb.assign(tr.snrDb.begin(), tr.snrDb.end());
    }

    DataConnector::Emit(info);
}

void PushRouter::HandleNewAdvert(const std::vector<uint8_t>& payload)
{
    DataConnector::PushNewAdvertInfo info {};
    info.payloadLen = payload.size();

    MeshCoreProto::ContactRecord rec {};
    info.valid = MeshCoreProto::decodeContactRecord(payload, rec);

    if (info.valid)
    {
        info.nodeId = rec.nodeId();
        info.type = rec.type;
        info.flags = rec.flags;
        info.name = rec.name;
        info.publicKey = rec.publicKey;
        info.prefix6 = rec.prefix6();
        info.lastAdvert = rec.lastAdvert;
        info.lastMod = rec.lastMod;
        info.advLatE6 = rec.advLatE6;
        info.advLonE6 = rec.advLonE6;
    }

    DataConnector::Emit(info);
    m_runtime.RequestContactSync();
}

void PushRouter::HandleUnknown(uint8_t code, const std::vector<uint8_t>& payload)
{
    if (code < 0x80)
    {
        return;
    }

    DataConnector::PushUnknownInfo info
    {
        code,
        payload.size()
    };

    DataConnector::Emit(info);
}