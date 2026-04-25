#include "MeshCoreFramer.h"
#include "IByteStream.h"
#include "SerialPort.h"

#include <chrono>
#include <unistd.h>

MeshCoreFramer::MeshCoreFramer(IByteStream &stream)
    : m_stream(stream)
{
}

uint16_t MeshCoreFramer::le16(uint8_t lo, uint8_t hi)
{
    return static_cast<uint16_t>(lo) | (static_cast<uint16_t>(hi) << 8);
}


bool MeshCoreFramer::sendPayload(const std::vector<uint8_t> &payload)
{
    if (!m_stream.isOpen())
    {
        return false;
    }

    if (payload.size() > 0xFFFF)
    {
        return false;
    }

    uint16_t len = static_cast<uint16_t>(payload.size());

    std::vector<uint8_t> frame;
    frame.reserve(1 + 2 + payload.size());

    frame.push_back(USB_INBOUND_PREFIX);
    frame.push_back(static_cast<uint8_t>(len & 0xFF));
    frame.push_back(static_cast<uint8_t>((len >> 8) & 0xFF));
    frame.insert(frame.end(), payload.begin(), payload.end());

    return m_stream.writeAll(frame.data(), frame.size());
}

std::optional<std::vector<uint8_t>> MeshCoreFramer::readPayload(int timeoutMs)
{
    if (!m_stream.isOpen())
    {
        return std::nullopt;
    }

    while (true)
    {
        uint8_t b = 0;

        if (!m_stream.readByte(b, timeoutMs))
        {
            return std::nullopt;
        }

        if (b != USB_OUTBOUND_PREFIX)
        {
            continue;
        }

        uint8_t lenBytes[2];

        if (!m_stream.readExact(lenBytes, 2, timeoutMs))
        {
            return std::nullopt;
        }

        uint16_t len = le16(lenBytes[0], lenBytes[1]);

        std::vector<uint8_t> payload(len);

        if (len > 0)
        {
            if (!m_stream.readExact(payload.data(), payload.size(), timeoutMs))
            {
                return std::nullopt;
            }
        }

        return payload;
    }
}