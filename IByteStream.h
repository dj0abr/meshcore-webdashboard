#pragma once

#include <cstddef>
#include <cstdint>

class IByteStream
{
public:
    virtual ~IByteStream() = default;

    virtual void close() = 0;
    virtual bool isOpen() const = 0;

    virtual bool writeAll(const uint8_t *data, size_t len) = 0;
    virtual bool readExact(uint8_t *data, size_t len, int timeoutMs) = 0;
    virtual bool readByte(uint8_t &byte, int timeoutMs) = 0;
};