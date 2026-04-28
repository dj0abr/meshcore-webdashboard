#include "PushRouter.h"

#include "DataConnector.h"
#include "MeshCoreProto.h"
#include "MeshDB.h"
#include "MeshRxLogDecoder.h"
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

void PushRouter::HandleLogRxData(const std::vector<uint8_t>& payload)
{
    std::cout << "[DEBUG RX_LOG] HandleLogRxData called" << std::endl;
    std::cout << "[DEBUG RX_LOG] payload.size=" << payload.size() << std::endl;

    std::cout << "[DEBUG RX_LOG] RAW: ";
    for (uint8_t b : payload)
    {
        printf("%02X ", b);
    }
    printf("\n");

    const MeshRxLogDecoder::DecodedPacket pkt =
        MeshRxLogDecoder::Decode(payload);

    std::cout << "[DEBUG RX_LOG] Decode result:" << std::endl;
    std::cout << "  valid=" << pkt.valid << std::endl;
    std::cout << "  pushCode=" << static_cast<int>(pkt.pushCode) << std::endl;
    std::cout << "  routeType=" << static_cast<int>(pkt.routeType) << std::endl;
    std::cout << "  payloadType=" << static_cast<int>(pkt.payloadType) << std::endl;
    std::cout << "  payloadVersion=" << static_cast<int>(pkt.payloadVersion) << std::endl;

    std::cout << "  snrDb=" << pkt.snrDb << std::endl;
    std::cout << "  rssiDbm=" << pkt.rssiDbm << std::endl;

    std::cout << "  pathLen=" << static_cast<int>(pkt.pathLen) << std::endl;
    std::cout << "  pathHashSize=" << static_cast<int>(pkt.pathHashSize) << std::endl;
    std::cout << "  pathText=" << MeshRxLogDecoder::FormatPath(pkt) << std::endl;

    std::cout << "  pktHash=" << pkt.pktHash << std::endl;
    std::cout << "  originalHex.size=" << pkt.originalHex.size() << std::endl;

    std::cout << "  grpTxtValid=" << pkt.grpTxtValid << std::endl;
    std::cout << "  grpResolvedChannelName=" << pkt.grpResolvedChannelName << std::endl;
    std::cout << "  grpTimestamp=" << pkt.grpTimestamp << std::endl;
    std::cout << "  grpTxtType=" << static_cast<int>(pkt.grpTxtType) << std::endl;
    std::cout << "  grpText=" << pkt.grpText << std::endl;

    std::cout << "  advertValid=" << pkt.advertValid << std::endl;
    std::cout << "  advertPublicKey.size=" << pkt.advertPublicKey.size() << std::endl;

    if (!pkt.advertPublicKey.empty())
    {
        std::cout << "  advertPublicKey="
                  << MeshRxLogDecoder::BytesToHex(pkt.advertPublicKey)
                  << std::endl;
    }

    std::cout << "  advertTimestamp=" << pkt.advertTimestamp << std::endl;
    std::cout << "  advertRole=" << static_cast<int>(pkt.advertRole) << std::endl;
    std::cout << "  advertHasGps=" << pkt.advertHasGps << std::endl;
    std::cout << "  advertHasBle=" << pkt.advertHasBle << std::endl;
    std::cout << "  advertHasShortcut=" << pkt.advertHasShortcut << std::endl;
    std::cout << "  advertHasName=" << pkt.advertHasName << std::endl;
    std::cout << "  advertLocationValid=" << pkt.advertLocationValid << std::endl;
    std::cout << "  advertLatitudeE6=" << pkt.advertLatitudeE6 << std::endl;
    std::cout << "  advertLongitudeE6=" << pkt.advertLongitudeE6 << std::endl;
    std::cout << "  advertName=" << pkt.advertName << std::endl;

    // Payload 5 = Grp-Text, also Messages, alles andere ist uninteressant (zumindest vorerst)
    if (pkt.payloadType != 5 && pkt.payloadType != 4)
    {
        std::cout << "[DEBUG RX_LOG] DROP: payloadType != 4 bzw. 5" << std::endl;
        return;
    }

    DataConnector::PushRxLogInfo info {};
    info.valid = pkt.valid;
    info.payloadLen = payload.size();

    if (!pkt.valid)
    {
        std::cout << "[DEBUG RX_LOG] DROP/EMIT invalid packet" << std::endl;
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

    std::cout << "[DEBUG RX_LOG] info prepared:" << std::endl;
    std::cout << "  info.pathLen=" << info.pathLen << std::endl;
    std::cout << "  info.pathHashSize=" << info.pathHashSize << std::endl;
    std::cout << "  info.pathText=" << info.pathText << std::endl;
    std::cout << "  info.rawHex.size=" << info.rawHex.size() << std::endl;

    if (pkt.grpTxtValid)
    {
        std::cout << "[DEBUG RX_LOG] Processing group text" << std::endl;

        if (!pkt.grpResolvedChannelName.empty())
        {
            std::cout << "  lookup channel name="
                      << pkt.grpResolvedChannelName
                      << std::endl;

            const auto channelRec = MeshDB::FindChannelByName(pkt.grpResolvedChannelName);

            if (channelRec.has_value())
            {
                info.channelIdx = channelRec->channelIdx;
                info.hasChannelIdx = true;

                std::cout << "  channelIdx=" << info.channelIdx << std::endl;
            }
            else
            {
                std::cout << "  channel lookup failed" << std::endl;
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

            std::cout << "  correlationKey=" << info.correlationKey << std::endl;
        }
    }

    if (pkt.advertValid)
    {
        std::cout << "[DEBUG RX_LOG] Processing advert inside RX_LOG" << std::endl;

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

            std::cout << "  info.advertPublicKey="
                      << info.advertPublicKey
                      << std::endl;
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

            std::cout << "  info.advertLatitudeE6=" << info.advertLatitudeE6 << std::endl;
            std::cout << "  info.advertLongitudeE6=" << info.advertLongitudeE6 << std::endl;
        }

        if (pkt.advertHasName && !pkt.advertName.empty())
        {
            info.advertName = pkt.advertName;
            info.hasAdvertName = true;

            std::cout << "  info.advertName=" << info.advertName << std::endl;
        }
    }

    std::cout << "[DEBUG RX_LOG] Emit(info)" << std::endl;
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