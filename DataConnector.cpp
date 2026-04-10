#include "DataConnector.h"
#include "MeshDB.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>

bool DataConnector::s_consoleOutputEnabled = true;

static const char* advertTypeStrings[] =
{
    "NONE",
    "CHAT",
    "REPEATER",
    "ROOM",
    "SENSOR"
};

std::string DataConnector::u32hex(uint32_t v)
{
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0') << std::setw(8) << v;
    return oss.str();
}

const char* DataConnector::AdvertTypeToStr(AdvertType t)
{
    uint8_t idx = static_cast<uint8_t>(t);

    if (idx >= static_cast<uint8_t>(AdvertType::COUNT))
    {
        return "UNKNOWN";
    }

    return advertTypeStrings[idx];
}

std::string DataConnector::formatLocalTime(uint32_t epoch)
{
    std::time_t t = static_cast<std::time_t>(epoch);
    std::tm tmLocal {};
    localtime_r(&t, &tmLocal);

    char buf[64];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmLocal);
    return std::string(buf);
}

std::string DataConnector::hexBytes(const uint8_t* p, size_t n)
{
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0');

    for (size_t i = 0; i < n; i++)
    {
        oss << std::setw(2) << static_cast<unsigned>(p[i]);
    }

    return oss.str();
}

void DataConnector::EnableConsoleOutput(bool enable)
{
    s_consoleOutputEnabled = enable;
}

void DataConnector::WriteConsoleLine(const std::string& line)
{
    if (!s_consoleOutputEnabled)
    {
        return;
    }

    std::cout << line << "\n";
}

DataConnector::EventType DataConnector::GetEventType(const AdvertInfo&)
{
    return EventType::Advert;
}

DataConnector::EventType DataConnector::GetEventType(const MessageInfo&)
{
    return EventType::Message;
}

DataConnector::EventType DataConnector::GetEventType(const PushAdvertInfo&)
{
    return EventType::PushAdvert;
}

DataConnector::EventType DataConnector::GetEventType(const PushPathUpdatedInfo&)
{
    return EventType::PushPathUpdated;
}

DataConnector::EventType DataConnector::GetEventType(const PushSendConfirmedInfo&)
{
    return EventType::PushSendConfirmed;
}

DataConnector::EventType DataConnector::GetEventType(const PushSimpleInfo&)
{
    return EventType::PushSimple;
}

DataConnector::EventType DataConnector::GetEventType(const PushTraceInfo&)
{
    return EventType::PushTrace;
}

DataConnector::EventType DataConnector::GetEventType(const PushNewAdvertInfo&)
{
    return EventType::PushNewAdvert;
}

DataConnector::EventType DataConnector::GetEventType(const PushUnknownInfo&)
{
    return EventType::PushUnknown;
}

DataConnector::EventType DataConnector::GetEventType(const PushRxLogInfo&)
{
    return EventType::PushSimple;
}

std::string DataConnector::FormatAdvert(const AdvertInfo& info)
{
    std::ostringstream oss;

    //const uint8_t rawType = static_cast<uint8_t>(info.type);
    //printf("raw: %d\n",rawType);

    oss
        << " - " << u32hex(info.nodeId)
        << "  type=" << AdvertTypeToStr(info.type)
        << "  flags=0x" << std::hex << unsigned(info.flags) << std::dec
        << "  last_seen=" << formatLocalTime(info.lastAdvert)
        << "  name=\"" << info.name << "\""
        << "  pk32=" << hexBytes(info.publicKey.data(), 6) << "...";

    return oss.str();
}

std::string DataConnector::FormatMessage(const MessageInfo& info)
{
    std::ostringstream oss;

    switch (info.kind)
    {
        case MessageInfo::Kind::Room:
        {
            oss
                << "[ROOM] room=" << info.fromName
                << " sender=" << (info.roomSenderName.empty() ? "<unknown>" : info.roomSenderName)
                << " pfx=" << hexBytes(info.senderPrefix6.data(), info.senderPrefix6.size())
                << " ts=" << info.senderTimestamp;
            break;
        }

        case MessageInfo::Kind::Channel:
        {
            oss
                << "[CHANNEL] ch=" << unsigned(info.channelIdx)
                << " ts=" << info.senderTimestamp;
            break;
        }

        case MessageInfo::Kind::Direct:
        default:
        {
            oss
                << "[DM] from=" << info.fromName
                << " pfx=" << hexBytes(info.senderPrefix6.data(), info.senderPrefix6.size())
                << " ts=" << info.senderTimestamp;
            break;
        }
    }

    if (!std::isnan(info.snrDb))
    {
        oss << " snr=" << info.snrDb;
    }

    oss
        << " path=" << unsigned(info.pathLen)
        << " type=" << unsigned(info.txtType)
        << "\n    " << info.text;

    return oss.str();
}

std::string DataConnector::FormatPush(const PushAdvertInfo& info)
{
    std::ostringstream oss;

    if (info.valid)
    {
        oss
            << "[PUSH ADVERT] "
            << info.name
            << " pfx=" << hexBytes(info.prefix6.data(), info.prefix6.size());
    }
    else
    {
        oss
            << "[PUSH ADVERT] len=" << info.payloadLen;
    }

    return oss.str();
}

std::string DataConnector::FormatPush(const PushPathUpdatedInfo& info)
{
    std::ostringstream oss;

    if (info.valid)
    {
        oss
            << "[PUSH PATH_UPDATED] pk32="
            << hexBytes(info.publicKey.data(), 6)
            << "...";
    }
    else
    {
        oss
            << "[PUSH PATH_UPDATED] len=" << info.payloadLen;
    }

    return oss.str();
}

std::string DataConnector::FormatPush(const PushSendConfirmedInfo& info)
{
    std::ostringstream oss;

    if (info.valid)
    {
        oss
            << "[PUSH SEND_CONFIRMED] ack=0x" << u32hex(info.ack)
            << " rtt_ms=" << info.rttMs;
    }
    else
    {
        oss
            << "[PUSH SEND_CONFIRMED] len=" << info.payloadLen;
    }

    return oss.str();
}

std::string DataConnector::FormatPush(const PushSimpleInfo& info)
{
    std::ostringstream oss;
    oss << "[" << info.label << "] len=" << info.payloadLen;
    return oss.str();
}

std::string DataConnector::FormatPush(const PushTraceInfo& info)
{
    std::ostringstream oss;

    if (info.valid)
    {
        oss
            << "[PUSH TRACE] flags=0x" << std::hex << unsigned(info.flags) << std::dec
            << " tag=0x" << u32hex(info.tag)
            << " auth=0x" << u32hex(info.authCode)
            << " hashes=";

        for (auto b : info.pathHashes)
        {
            oss
                << std::uppercase
                << std::hex
                << std::setw(2)
                << std::setfill('0')
                << unsigned(b);
        }

        oss << std::dec << " snr=";

        for (size_t i = 0; i < info.snrDb.size(); i++)
        {
            if (i != 0)
            {
                oss << ",";
            }

            oss << info.snrDb[i];
        }
    }
    else
    {
        oss << "[PUSH TRACE] len=" << info.payloadLen;
    }

    return oss.str();
}

std::string DataConnector::FormatPush(const PushNewAdvertInfo& info)
{
    std::ostringstream oss;

    if (info.valid)
    {
        oss
            << "[PUSH NEW_ADVERT] "
            << info.name
            << " pfx=" << hexBytes(info.prefix6.data(), info.prefix6.size())
            << " last_seen=" << formatLocalTime(info.lastAdvert)
            << " last_mod=" << formatLocalTime(info.lastMod);
    }
    else
    {
        oss << "[PUSH NEW_ADVERT] len=" << info.payloadLen;
    }

    return oss.str();
}

std::string DataConnector::FormatPush(const PushUnknownInfo& info)
{
    std::ostringstream oss;

    oss
        << "[PUSH 0x" << std::hex << unsigned(info.code) << std::dec
        << "] len=" << info.payloadLen;

    return oss.str();
}

std::string DataConnector::FormatPush(const PushRxLogInfo& info)
{
    std::ostringstream oss;

    oss << "[PUSH RX_LOG]";

    if (!info.valid)
    {
        oss << " len=" << info.payloadLen;
        return oss.str();
    }

    oss
        << " routeType=" << unsigned(info.routeType)
        << " payloadType=" << unsigned(info.payloadType)
        << " payloadVersion=" << unsigned(info.payloadVersion);

    if (info.hasChannelIdx)
    {
        oss << " ch=" << unsigned(info.channelIdx);
    }

    if (info.hasSenderTimestamp)
    {
        oss << " ts=" << info.senderTimestamp;
    }

    if (info.hasTxtType)
    {
        oss << " txtType=" << unsigned(info.txtType);
    }

    if (info.hasSnrDb)
    {
        oss << " snr=" << info.snrDb;
    }

    if (info.hasRssiDbm)
    {
        oss << " rssi=" << info.rssiDbm;
    }

    if (info.hasPathLen)
    {
        oss << " pathLen=" << unsigned(info.pathLen);
    }

    if (!info.pathText.empty())
    {
        oss << " path=" << info.pathText;
    }

    if (info.hasPktHash)
    {
        oss << " pktHash=0x" << u32hex(info.pktHash);
    }

    if (info.hasAdvert)
    {
        oss << " advert";

        if (info.hasAdvertName)
        {
            oss << " name=" << info.advertName;
        }

        if (info.hasAdvertRole)
        {
            oss << " role=" << unsigned(info.advertRole);
        }

        if (info.hasAdvertPublicKey)
        {
            oss << " pk=" << info.advertPublicKey.substr(0, 12) << "...";
        }

        if (info.hasAdvertLatitudeE6 && info.hasAdvertLongitudeE6)
        {
            oss
                << " latE6=" << info.advertLatitudeE6
                << " lonE6=" << info.advertLongitudeE6;
        }
    }

    if (info.hasMessageText)
    {
        oss << "\n    " << info.messageText;
    }

    return oss.str();
}

void DataConnector::EmitFormatted(EventType eventType, const std::string& line)
{
    (void)eventType;

    WriteConsoleLine(line);

    // Die eigentliche DB-Speicherung passiert in den typisierten Emit(...)
    // Overloads, weil wir dort noch die strukturierten Felder haben.
}

void DataConnector::Emit(const AdvertInfo& info)
{
    const std::string line = FormatAdvert(info);
//printf("Adv: %s\n",line.c_str())    ;
    MeshDB::StoreAdvert(info, line);
    EmitFormatted(GetEventType(info), line);
}

void DataConnector::Emit(const MessageInfo& info)
{
    std::cout << "[DataConnector] Emit MessageInfo:"
          << " kind=" << unsigned(static_cast<uint8_t>(info.kind))
          << " channelIdx=" << unsigned(info.channelIdx)
          << " fromName=[" << info.fromName << "]"
          << " text=[" << info.text << "]"
          << "\n";
          
    const std::string line = FormatMessage(info);
    MeshDB::StoreMessage(info, line);
    EmitFormatted(GetEventType(info), line);
}

void DataConnector::Emit(const PushAdvertInfo& info)
{
    const std::string line = FormatPush(info);
    MeshDB::StorePushAdvert(info, line);
    EmitFormatted(GetEventType(info), line);
}

void DataConnector::Emit(const PushPathUpdatedInfo& info)
{
    const std::string line = FormatPush(info);
    MeshDB::StorePushPathUpdated(info, line);
    EmitFormatted(GetEventType(info), line);
}

void DataConnector::Emit(const PushSendConfirmedInfo& info)
{
    const std::string line = FormatPush(info);
    MeshDB::StorePushSendConfirmed(info, line);
    EmitFormatted(GetEventType(info), line);
}

void DataConnector::Emit(const PushSimpleInfo& info)
{
    const std::string line = FormatPush(info);
    MeshDB::StorePushSimple(info, line);
    EmitFormatted(GetEventType(info), line);
}

void DataConnector::Emit(const PushTraceInfo& info)
{
    const std::string line = FormatPush(info);
    MeshDB::StorePushTrace(info, line);
    EmitFormatted(GetEventType(info), line);
}

void DataConnector::Emit(const PushNewAdvertInfo& info)
{
    const std::string line = FormatPush(info);
    MeshDB::StorePushNewAdvert(info, line);
    EmitFormatted(GetEventType(info), line);
}

void DataConnector::Emit(const PushUnknownInfo& info)
{
    const std::string line = FormatPush(info);
    MeshDB::StorePushUnknown(info, line);
    EmitFormatted(GetEventType(info), line);
}

void DataConnector::Emit(const RoomPasswordRequiredInfo& info)
{
    std::cout << "[ROOM] password required"
              << " room=" << info.roomName
              << " node=" << info.roomNodeId
              << "\n";
}

void DataConnector::Emit(const PushRxLogInfo& info)
{
    const std::string line = FormatPush(info);
    MeshDB::StorePushRxLog(info, line);
    EmitFormatted(GetEventType(info), line);
}
