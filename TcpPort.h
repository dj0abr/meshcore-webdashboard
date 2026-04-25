#pragma once

#include "IByteStream.h"

#include <cstddef>
#include <cstdint>
#include <string>

class TcpPort : public IByteStream
{
public:
    TcpPort();
    ~TcpPort() override;

    bool open(const std::string &host, uint16_t port);

    void close() override;
    bool isOpen() const override;

    bool writeAll(const uint8_t *data, size_t len) override;
    bool readExact(uint8_t *data, size_t len, int timeoutMs) override;
    bool readByte(uint8_t &byte, int timeoutMs) override;

private:
    int m_fd;
};