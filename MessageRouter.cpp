#include "MessageRouter.h"
#include "DataConnector.h"
#include "MeshDB.h"
#include "MessageCorrelation.h"

#include <iostream>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <cctype>

namespace
{
    std::string Normalize(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c)
        {
            return static_cast<char>(std::tolower(c));
        });

        // alles was kein Buchstabe/Zahl ist → Leerzeichen
        std::replace_if(s.begin(), s.end(), [](unsigned char c)
        {
            return !std::isalnum(c);
        }, ' ');

        return s;
    }

    bool ContainsWord(const std::string& text, const std::string& word)
    {
        const std::string norm = Normalize(text);
        const std::string w = word;

        return norm.find(w) != std::string::npos;
    }

    bool IsTestCommand(const std::string& text)
    {
        return ContainsWord(text, "ping")
            || ContainsWord(text, "test")
            || ContainsWord(text, "path");
    }
}

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

    // Autoresponder in channel #test
    if (msg.isChannel && fromName == "#test")
    {
        HandleTestChannelMessage(msg);
    }

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

static std::string ExtractSenderName(const std::string& text)
{
    const auto pos = text.find(':');

    if (pos == std::string::npos)
    {
        return {};
    }

    return text.substr(0, pos);
}

void MessageRouter::HandleTestChannelMessage(const MeshCoreClient::RxMessage& msg)
{
    std::cout << "[#test] " << msg.text << "\n";

    if (!IsTestCommand(msg.text)) return;

    // keine Ping Flood zulassen
    static std::time_t lastReply = 0;
    const std::time_t now = std::time(nullptr);
    if ((now - lastReply) < 10)
    {
        std::cout << "[#test] auto reply suppressed by cooldown\n";
        return;
    }
    lastReply = now;

    const std::string senderName = ExtractSenderName(msg.text);

    std::string reply =                 "";
    if (!senderName.empty()) reply +=   "@[" + senderName + "] 🏓 Pong";
    reply +=                            " | Hops: " + std::to_string(static_cast<unsigned>(msg.pathLen));

    if (!MeshDB::EnqueueChannelTxFromBot(msg.channelIdx, reply))
    {
        std::cout << "[#test] enqueue auto reply failed\n";
        return;
    }

    std::cout << "[#test] auto reply queued: " << reply << "\n";
}
