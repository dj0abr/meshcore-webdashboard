#include "MessageRouter.h"
#include "DataConnector.h"
#include "MeshDB.h"
#include "MessageCorrelation.h"

#include <iostream>

MessageRouter::MessageRouter(MeshCoreClient& client)
    : m_client(client)
{
}

void MessageRouter::Attach()
{
    std::cout << "[MessageRouter] Attach called this=" << this << "\n";

    m_client.setMessageCallback(
        [this](const MeshCoreClient::RxMessage& msg, const std::string& fromName)
        {
            std::cout << "[MessageRouter] callback firing this=" << this
                << " isChannel=" << (msg.isChannel ? 1 : 0)
                << " channelIdx=" << unsigned(msg.channelIdx)
                << "\n";
            HandleMessage(msg, fromName);
        });
}

void MessageRouter::HandleMessage(const MeshCoreClient::RxMessage& msg, const std::string& fromName)
{
    std::cout   << "[MessageRouter] HandleMessage:"
                << " isChannel=" << (msg.isChannel ? 1 : 0)
                << " channelIdx=" << unsigned(msg.channelIdx)
                << " txtType=" << unsigned(msg.txtType)
                << " pathLen=" << unsigned(msg.pathLen)
                << " snrDb=" << msg.snrDb
                << " senderTimestamp=" << msg.senderTimestamp
                << " fromName=[" << fromName << "]"
                << " textLen=" << msg.text.size()
                << " text=[" << msg.text << "]"
                << "\n";

    DataConnector::MessageInfo info {};

    info.isChannel = msg.isChannel;
    info.fromName = fromName;
    info.senderPrefix6 = msg.senderPrefix6;
    info.senderTimestamp = msg.senderTimestamp;
    info.snrDb = msg.snrDb;
    info.pathLen = msg.pathLen;
    info.txtType = msg.txtType;
    info.channelIdx = msg.channelIdx;
    info.text = msg.text;

    if (msg.isChannel)
    {
        info.kind = DataConnector::MessageInfo::Kind::Channel;
    }
    else if (msg.txtType == 2)
    {
        info.kind = DataConnector::MessageInfo::Kind::Room;

        if (info.text.size() >= 4)
        {
            std::array<uint8_t, 4> prefix4 {};

            prefix4[0] = static_cast<uint8_t>(info.text[0]);
            prefix4[1] = static_cast<uint8_t>(info.text[1]);
            prefix4[2] = static_cast<uint8_t>(info.text[2]);
            prefix4[3] = static_cast<uint8_t>(info.text[3]);

            const auto resolvedName = MeshDB::FindNodeNameByPrefix4(prefix4);

            if (resolvedName.has_value() && !resolvedName->empty())
            {
                printf(
                    "ROOM msg: room=%s sender=%s\n",
                    info.fromName.c_str(),
                    info.roomSenderName.empty() ? "<unknown>" : info.roomSenderName.c_str()
                );

                info.roomSenderName = *resolvedName;
            }

            info.text.erase(0, 4);
        }
    }
    else
    {
        info.kind = DataConnector::MessageInfo::Kind::Direct;
    }

    if (info.kind == DataConnector::MessageInfo::Kind::Channel)
    {
        info.correlationKey = MessageCorrelation::BuildKey(
            info.channelIdx,
            info.senderTimestamp,
            info.txtType,
            info.text);
    }

    std::cout << "[MessageRouter] emitting MessageInfo:"
              << " kind=" << unsigned(static_cast<uint8_t>(info.kind))
              << " channelIdx=" << unsigned(info.channelIdx)
              << " fromName=[" << info.fromName << "]"
              << " text=[" << info.text << "]";

    if (!info.correlationKey.empty())
    {
        std::cout << " correlationKey=" << info.correlationKey;
    }

    std::cout << "\n";

    DataConnector::Emit(info);
}