#include "MessageRouter.h"
#include "DataConnector.h"
#include "MeshDB.h"
#include "MessageCorrelation.h"

#include <iostream>
#include <cmath>
#include <ctime>
#include <algorithm>
#include <cctype>
#include <array>

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
        return ContainsWord(text, "ping");
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
    if (!MeshDB::IsCompanionBotEnabled()) return;

    std::cout << "[#test] " << msg.text << "\n";

    if (!IsTestCommand(msg.text)) return;

    // beantworte Ping nur für 2 oder 3 Byte Hashes
    if (msg.pathHashSize != 2 && msg.pathHashSize != 3) return;

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
    const std::string locName    = MeshDB::GetCompanionLocationName();
    const std::string hashBytes  = std::to_string(static_cast<unsigned>(msg.pathHashSize));

    static const std::array<std::string, 16> prefixes =
    {
        " 👍{hash}👍Byte Hash: ",
        " 👍{hash}B👍 Hash sagt: ",
        " [{hash} Byte Hash] ",
        " Hash {hash}B: ",
        " via {hash} Byte Hash: ",
        " {hash}B Hash empfangen: ",
        " über {hash} Byte Hash: ",
        " mit {hash}B Hash: ",
        " {hash}-Byte-Hash sagt: ",
        " 👍{hash} Byte Hash👍 ",
        " Hashgröße {hash} Byte: ",
        " per {hash}B Hash: ",
        " {hash} Byte Route: ",
        " Route mit {hash}B Hash: ",
        " Mesh {hash}B Hash: ",
        " Testkanal {hash}B: "
    };

    static const std::array<std::string, 70> replies =
    {
        "{name}, du bist gehört in {loc} mit {hops} hops",
        "Hallo {name}, klappt bis {loc} mit {hops} hops",
        "{name}, kommst hier in {loc} gut an über {hops} hops",
        "hier in {loc} alles lesbar mit {hops} hops, {name}",
        "Signal in {loc} sauber angekommen über {hops} hops, {name}",
        "{name}, kommt gut rüber nach {loc} mit {hops} hops",
        "in {loc} noch gut verständlich über {hops} hops, {name}",
        "{name}, läuft bis {loc} stabil mit {hops} hops",
        "hier in {loc} problemlos empfangen über {hops} hops, {name}",
        "empfang in {loc} bestätigt, {name}, {hops} hops",
        "{name}, kommt hier in {loc} noch ordentlich an mit {hops} hops",
        "alles gut bis {loc} über {hops} hops, {name}",
        "hier aus {loc}, passt mit {hops} hops, {name}",
        "{name}, sauber durch bis {loc} mit {hops} hops",
        "in {loc} gut angekommen mit {hops} hops, {name}",
        "lese dich hier in {loc} über {hops} hops, {name}",
        "bis {loc} noch voll ok, hast {hops} hops, {name}",
        "hier in {loc} ohne probleme empfangen",
        "signal kam hier in {loc} an mit {hops} hops, {name}",
        "in {loc} alles gut lesbar, hat {hops} x gehoppt, {name}",
        "servus {name} aus {loc}, dein signal passt mit {hops} hops",
        "hier in {loc} sauber empfangen über {hops} hops, {name}",
        "dein paketl is bis {loc} kemma, {name}, mit {hops} hops",
        "läuft guad bis {loc} über {hops} hops, {name}",
        "hier in {loc} no richtig guad lesbar mit {hops} hops, {name}",
        "{name}, kommt in {loc} astrein an über {hops} hops",
        "sauberer empfang hier in {loc} mit {hops} hops, {name}",
        "dein signal hat {hops} hops bis {loc} gebraucht, {name}",
        "hier aus {loc}, alles im grünen bereich",
        "bist in {loc} no deutlich zu hören mit {hops} hops, {name}",
        "kommt hier in {loc} guad owe über {hops} hops, {name}",
        "signal hier in {loc} ohne aussetzer mit {hops} hops, {name}",
        "hier in {loc} vui guad verständlich über {hops} hops, {name}",
        "{name}, läuft sauber ein nach {loc} mit {hops} hops",
        "dein signal is in {loc} angekommen über {hops} hops, {name}",
        "hier in {loc} passt ois mit {hops} hops, {name}",
        "bis {loc} noch stabil mit {hops} hops, {name}",
        "hier in {loc}, Empfang top über {hops} hops, {name}",
        "{name}, kommt locker bis {loc} rüber mit {hops} hops",
        "hier in {loc} noch gut zu dekodieren über {hops} hops, {name}",
        "dein signal kommt bis {loc} sauber durch mit {hops} hops, {name}",
        "aus {loc} alles bestens empfangen über {hops} hops, {name}",
        "hier in {loc} klar und deutlich mit {hops} hops, {name}",
        "bis {loc} ohne probleme angekommen über {hops} hops, {name}",
        "dein paket ist hier in {loc} lesbar mit {hops} hops, {name}",
        "empfang aus {loc} bestätigt, servus {name}",
        "hier in {loc} noch volle kopie mit {hops} hops, {name}",
        "{name}, kommt in {loc} sauber an mit {hops} hops",
        "signalweg bis {loc} erfolgreich über {hops} hops, {name}",
        "hier in {loc} alles verständlich angekommen mit {hops} hops, {name}",
        "dein signal hat {loc} erreicht über {hops} hops, {name}",
        "aus {loc} kann ich dich gut lesen mit {hops} hops, {name}",
        "hier in {loc} problemlos dekodiert über {hops} hops, {name}",
        "{name}, kommt bis {loc} noch ordentlich an mit {hops} hops",
        "signal in {loc} einwandfrei empfangen über {hops} hops, {name}",
        "hier in {loc} alles im lot mit {hops} hops, {name}",
        "dein paket kam nach {loc} mit {hops} hops, {name}",
        "lesbarkeit in {loc} gegeben über {hops} hops, {name}",
        "hier in {loc} sauber aufgenommen mit {hops} hops, {name}",
        "ankunft in {loc} bestätigt über {hops} hops, {name}",
        "dein signal is bis {loc} durchkemma mit {hops} hops, {name}",
        "hier in {loc} no glasklar lesbar über {hops} hops, {name}",
        "kommt bis {loc} ohne verluste an mit {hops} hops, {name}",
        "hier in {loc} alles bestens, {hops} hops, {name}",
        "signal aus der runde hier in {loc} angekommen",
        "dein paket hat {loc} erfolgreich erreicht mit {hops} hops, {name}",
        "hier in {loc} guter empfang trotz {hops} hops, {name}",
        "kommt in {loc} noch sauber lesbar an über {hops} hops, {name}",
        "servus {name} aus {loc}, signal passt einwandfrei mit {hops} hops",
        "hier in {loc} sauber lesbar, servus {name} über {hops} hops"
    };

    static std::size_t prefixIndex = 0;
    static std::size_t replyIndex  = 0;

    std::string prefix = prefixes[prefixIndex];
    std::string reply  = replies[replyIndex];

    if (++prefixIndex >= prefixes.size()) prefixIndex = 0;
    if (++replyIndex >= replies.size()) replyIndex = 0;

    auto ReplaceAll =
    [](std::string& str, const std::string& from, const std::string& to)
    {
        std::size_t pos = 0;

        while ((pos = str.find(from, pos)) != std::string::npos)
        {
            str.replace(pos, from.length(), to);
            pos += to.length();
        }
    };

    ReplaceAll(prefix, "{hash}", hashBytes);
    ReplaceAll(reply, "{name}", senderName);
    ReplaceAll(reply, "{loc}", locName);
    ReplaceAll(reply, "{hops}", std::to_string(static_cast<unsigned>(msg.pathLen)));

    reply = prefix + reply;

    if (!MeshDB::EnqueueChannelTxFromBot(msg.channelIdx, reply))
    {
        std::cout << "[#test] enqueue auto reply failed\n";
        return;
    }

    std::cout << "[#test] auto reply queued: " << reply << "\n";
}