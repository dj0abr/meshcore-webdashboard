#ifndef DATACONNECTOR_H
#define DATACONNECTOR_H

#include <string>
#include <array>
#include <vector>
#include <cstdint>
#include <ctime>
#include <limits>

class DataConnector
{
public:

    enum class EventType : uint8_t
    {
        Advert,
        Message,
        PushAdvert,
        PushPathUpdated,
        PushSendConfirmed,
        PushSimple,
        PushTrace,
        PushNewAdvert,
        PushUnknown
    };

    enum class AdvertType : uint8_t
    {
        NONE = 0,
        CHAT,
        REPEATER,
        ROOM,
        SENSOR,
        COUNT
    };

    struct AdvertInfo
    {
        uint32_t nodeId {};
        AdvertType type {};
        uint8_t flags {};
        std::time_t lastAdvert {};
        std::string name;
        std::array<uint8_t, 32> publicKey {};

        int32_t advLatE6 {};
        int32_t advLonE6 {};
    };

    struct MessageInfo
    {
        enum class Kind : uint8_t
        {
            Direct = 0,
            Room = 1,
            Channel = 2
        };

        Kind kind = Kind::Direct;
        bool isChannel = false;
        std::string fromName;
        std::string roomSenderName;
        std::array<uint8_t, 6> senderPrefix6 {};
        uint32_t senderTimestamp = 0;
        float snrDb = std::numeric_limits<float>::quiet_NaN();
        uint8_t pathLen = 255;
        uint8_t txtType = 0;
        uint8_t channelIdx = 0;
        std::string text;
    };

    struct PushAdvertInfo
    {
        bool valid;
        std::string name;
        std::array<uint8_t, 6> prefix6;
        size_t payloadLen;
    };

    struct PushPathUpdatedInfo
    {
        bool valid;
        std::array<uint8_t, 32> publicKey;
        size_t payloadLen;
    };

    struct PushSendConfirmedInfo
    {
        bool valid;
        uint32_t ack;
        uint32_t rttMs;
        size_t payloadLen;
    };

    struct PushSimpleInfo
    {
        std::string label;
        size_t payloadLen;
    };

    struct PushTraceInfo
    {
        bool valid;
        uint8_t flags;
        uint32_t tag;
        uint32_t authCode;
        std::vector<uint8_t> pathHashes;
        std::vector<float> snrDb;
        size_t payloadLen;
    };

    struct PushNewAdvertInfo
    {
        bool valid;
        uint32_t nodeId {};
        uint8_t type {};
        uint8_t flags {};
        std::string name;
        std::array<uint8_t, 32> publicKey {};
        std::array<uint8_t, 6> prefix6 {};
        uint32_t lastAdvert {};
        uint32_t lastMod {};
        int32_t advLatE6 {};
        int32_t advLonE6 {};
        size_t payloadLen {};
    };

    struct PushUnknownInfo
    {
        uint8_t code;
        size_t payloadLen;
    };

    struct RoomPasswordRequiredInfo
    {
        std::string roomName;
        uint32_t roomNodeId = 0;
    };

    static const char* AdvertTypeToStr(AdvertType t);

    static std::string FormatAdvert(const AdvertInfo& info);
    static std::string FormatMessage(const MessageInfo& info);

    static std::string FormatPush(const PushAdvertInfo& info);
    static std::string FormatPush(const PushPathUpdatedInfo& info);
    static std::string FormatPush(const PushSendConfirmedInfo& info);
    static std::string FormatPush(const PushSimpleInfo& info);
    static std::string FormatPush(const PushTraceInfo& info);
    static std::string FormatPush(const PushNewAdvertInfo& info);
    static std::string FormatPush(const PushUnknownInfo& info);

    static void Emit(const AdvertInfo& info);
    static void Emit(const MessageInfo& info);
    static void Emit(const PushAdvertInfo& info);
    static void Emit(const PushPathUpdatedInfo& info);
    static void Emit(const PushSendConfirmedInfo& info);
    static void Emit(const PushSimpleInfo& info);
    static void Emit(const PushTraceInfo& info);
    static void Emit(const PushNewAdvertInfo& info);
    static void Emit(const PushUnknownInfo& info);
    static void Emit(const RoomPasswordRequiredInfo& info);

    static std::string formatLocalTime(uint32_t epoch);
    static std::string hexBytes(const uint8_t* p, size_t n);
    static std::string u32hex(uint32_t v);

    static void EnableConsoleOutput(bool enable);

private:

    static void WriteConsoleLine(const std::string& line);
    static void EmitFormatted(EventType eventType, const std::string& line);

    static EventType GetEventType(const AdvertInfo&);
    static EventType GetEventType(const MessageInfo&);
    static EventType GetEventType(const PushAdvertInfo&);
    static EventType GetEventType(const PushPathUpdatedInfo&);
    static EventType GetEventType(const PushSendConfirmedInfo&);
    static EventType GetEventType(const PushSimpleInfo&);
    static EventType GetEventType(const PushTraceInfo&);
    static EventType GetEventType(const PushNewAdvertInfo&);
    static EventType GetEventType(const PushUnknownInfo&);

    static bool s_consoleOutputEnabled;

    DataConnector() = delete;
    ~DataConnector() = delete;
};

#endif