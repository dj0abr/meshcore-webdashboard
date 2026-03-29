#include "PushRouter.h"

#include "DataConnector.h"
#include "MeshCoreProto.h"
#include "MeshDB.h"

#include <array>
#include <iostream>

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
    switch (code)
    {
        case MeshCoreProto::PUSH_CODE_ADVERT:
        {
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

        case 0x88:
        {
            HandleLogRxData(payload);
            return;
        }

        case MeshCoreProto::PUSH_CODE_TRACE_DATA:
        {
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
    DataConnector::PushAdvertInfo info {};
    info.payloadLen = payload.size();

    std::array<uint8_t, 32> pk {};
    info.valid = MeshCoreProto::decodePublicKey32(payload, pk);

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
    if (!m_client.isRxLogEnabled())
    {
        return;
    }

    DataConnector::PushSimpleInfo info
    {
        "PUSH LOG_RX_DATA",
        payload.size()
    };

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