#pragma once

#include <cstdint>
#include <string>

namespace MessageCorrelation
{
    std::string BuildSource(
        uint8_t channelIdx,
        uint32_t senderTimestamp,
        uint8_t txtType,
        const std::string& text);

    std::string Sha256Hex(const std::string& input);

    std::string BuildKey(
        uint8_t channelIdx,
        uint32_t senderTimestamp,
        uint8_t txtType,
        const std::string& text);
}