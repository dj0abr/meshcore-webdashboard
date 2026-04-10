#include "MeshCoreProto.h"

#include <cstring>

namespace MeshCoreProto
{
    uint32_t le32(const uint8_t *p)
    {
        return (static_cast<uint32_t>(p[0]) << 0) |
               (static_cast<uint32_t>(p[1]) << 8) |
               (static_cast<uint32_t>(p[2]) << 16) |
               (static_cast<uint32_t>(p[3]) << 24);
    }

    uint32_t be32(const uint8_t *p)
    {
        return (static_cast<uint32_t>(p[0]) << 24) |
               (static_cast<uint32_t>(p[1]) << 16) |
               (static_cast<uint32_t>(p[2]) << 8) |
               (static_cast<uint32_t>(p[3]) << 0);
    }

    uint32_t ContactRecord::nodeId() const
    {
        return be32(publicKey.data());
    }

    std::array<uint8_t, 6> ContactRecord::prefix6() const
    {
        std::array<uint8_t, 6> p {};
        for (size_t i = 0; i < 6; i++)
        {
            p[i] = publicKey[i];
        }
        return p;
    }

    bool decodeContactRecord(const std::vector<uint8_t>& frame, ContactRecord& out)
    {
        if (frame.size() < 148)
        {
            return false;
        }

        for (size_t i = 0; i < 32; i++)
        {
            out.publicKey[i] = frame[1 + i];
        }

        out.type = frame[33];
        out.flags = frame[34];
        out.unknown1 = frame[35];

        for (size_t i = 0; i < 64; i++)
        {
            out.unknown64[i] = frame[36 + i];
        }

        const size_t advNameOff = 100;
        const size_t lastAdvertOff = advNameOff + 32;
        const size_t advLatOff = lastAdvertOff + 4;
        const size_t advLonOff = advLatOff + 4;
        const size_t lastModOff = advLonOff + 4;

        std::string name;
        name.reserve(32);

        for (size_t i = 0; i < 32; i++)
        {
            uint8_t c = frame[advNameOff + i];

            if (c == 0)
            {
                break;
            }

            name.push_back(static_cast<char>(c));
        }

        out.name = name;
        out.lastAdvert = le32(frame.data() + lastAdvertOff);
        out.advLatE6 = static_cast<int32_t>(le32(frame.data() + advLatOff));
        out.advLonE6 = static_cast<int32_t>(le32(frame.data() + advLonOff));
        out.lastMod = le32(frame.data() + lastModOff);

        return true;
    }

    bool decodePublicKey32(const std::vector<uint8_t> &frame, std::array<uint8_t, 32> &outPk)
    {
        if (frame.size() < 1 + 32)
        {
            return false;
        }

        for (size_t i = 0; i < 32; i++)
        {
            outPk[i] = frame[1 + i];
        }

        return true;
    }

    bool decodeTraceData(const std::vector<uint8_t> &frame, TraceData &out)
    {
        // Format (per MeshCore companion-radio protocol):
        // [0]=code (0x89)
        // [1]=reserved
        // [2]=path_len
        // [3]=flags
        // [4..7]=tag LE
        // [8..11]=auth_code LE
        // [12..12+path_len-1]=path_hashes
        // [12+path_len..12+2*path_len]=path_snrs (path_len+1 bytes), SNR = byte/4.0
        if (frame.size() < 13)
        {
            return false;
        }

        const uint8_t pathLen = frame[2];
        const size_t needLen = 13 + 2 * size_t(pathLen);
        if (frame.size() < needLen)
        {
            return false;
        }

        out.flags = frame[3];
        out.tag = le32(frame.data() + 4);
        out.authCode = le32(frame.data() + 8);

        out.pathHashes.assign(frame.begin() + 12, frame.begin() + 12 + pathLen);

        out.snrDb.clear();
        out.snrDb.reserve(size_t(pathLen) + 1);

        const uint8_t *snrBase = frame.data() + 12 + pathLen;
        for (size_t i = 0; i < size_t(pathLen) + 1; i++)
        {
            out.snrDb.push_back(static_cast<float>(snrBase[i]) / 4.0f);
        }

        return true;
    }
}

bool MeshCoreProto::decodeLoginSuccess(const std::vector<uint8_t>& frame, LoginSuccessInfo& out)
{
    if (frame.size() < 1 + 1 + 6 + 4)
    {
        return false;
    }

    if (frame[0] != PUSH_CODE_LOGIN_SUCCESS)
    {
        return false;
    }

    out.permissions = frame[1];

    for (size_t i = 0; i < 6; i++)
    {
        out.prefix6[i] = frame[2 + i];
    }

    out.tag = le32(frame.data() + 8);

    if (frame.size() >= 1 + 1 + 6 + 4 + 1)
    {
        out.newPermissions = frame[12];
        out.hasNewPermissions = true;
    }
    else
    {
        out.newPermissions = out.permissions;
        out.hasNewPermissions = false;
    }

    return true;
}

bool MeshCoreProto::decodeLoginSuccessPayload(const std::vector<uint8_t>& payload, LoginSuccessInfo& out)
{
    if (payload.size() < 1 + 6 + 4)
    {
        return false;
    }

    out.permissions = payload[0];

    for (size_t i = 0; i < 6; i++)
    {
        out.prefix6[i] = payload[1 + i];
    }

    out.tag = le32(payload.data() + 7);

    if (payload.size() >= 1 + 6 + 4 + 1)
    {
        out.newPermissions = payload[11];
        out.hasNewPermissions = true;
    }
    else
    {
        out.newPermissions = out.permissions;
        out.hasNewPermissions = false;
    }

    return true;
}

bool MeshCoreProto::decodeDiscoverResponse(const std::vector<uint8_t>& frame, DiscoverNode& out)
{
    out = DiscoverNode {};

    if (frame.empty())
    {
        return false;
    }

    if (frame[0] == MeshCoreProto::PUSH_CODE_RX_LOG_DATA)
    {
        if (frame.size() < 19)
        {
            return false;
        }

        if (frame[3] != 0x2E || frame[4] != 0x00 || frame[5] != 0x92)
        {
            return false;
        }

        out.valid = true;
        out.sourceCode = frame[0];
        out.snrRxDb = static_cast<float>(static_cast<int8_t>(frame[1])) / 4.0f;
        out.rssiDbm = static_cast<int>(static_cast<int8_t>(frame[2]));
        out.snrTxDb = static_cast<float>(static_cast<int8_t>(frame[6])) / 4.0f;
        out.tag = le32(frame.data() + 7);

        for (size_t i = 0; i < 8; ++i)
        {
            out.nodeId[i] = frame[11 + i];
        }

        return true;
    }

    if (frame[0] == 0x8E)
    {
        if (frame.size() < 18)
        {
            return false;
        }

        if (frame[3] != 0x00 || frame[4] != 0x92)
        {
            return false;
        }

        out.valid = true;
        out.sourceCode = frame[0];
        out.snrRxDb = static_cast<float>(static_cast<int8_t>(frame[1])) / 4.0f;
        out.rssiDbm = static_cast<int>(static_cast<int8_t>(frame[2]));
        out.snrTxDb = static_cast<float>(static_cast<int8_t>(frame[5])) / 4.0f;
        out.tag = le32(frame.data() + 6);

        for (size_t i = 0; i < 8; ++i)
        {
            out.nodeId[i] = frame[10 + i];
        }

        return true;
    }

    return false;
}