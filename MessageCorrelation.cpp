#include "MessageCorrelation.h"

#include <openssl/sha.h>

#include <iomanip>
#include <sstream>

namespace MessageCorrelation
{
    std::string BuildSource(
        uint8_t channelIdx,
        uint32_t senderTimestamp,
        uint8_t txtType,
        const std::string& text)
    {
        std::ostringstream oss;

        oss
            << "channel:" << unsigned(channelIdx)
            << "|ts:" << senderTimestamp
            << "|type:" << unsigned(txtType)
            << "|text:" << text;

        return oss.str();
    }

    std::string Sha256Hex(const std::string& input)
    {
        unsigned char hash[SHA256_DIGEST_LENGTH];

        SHA256(
            reinterpret_cast<const unsigned char*>(input.data()),
            input.size(),
            hash);

        std::ostringstream oss;

        for (size_t i = 0; i < SHA256_DIGEST_LENGTH; ++i)
        {
            oss
                << std::hex
                << std::setw(2)
                << std::setfill('0')
                << static_cast<unsigned>(hash[i]);
        }

        return oss.str();
    }

    std::string BuildKey(
        uint8_t channelIdx,
        uint32_t senderTimestamp,
        uint8_t txtType,
        const std::string& text)
    {
        return Sha256Hex(
            BuildSource(
                channelIdx,
                senderTimestamp,
                txtType,
                text));
    }
}