#include "TcpPort.h"

#include <arpa/inet.h>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <netdb.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

TcpPort::TcpPort()
{
    m_fd = -1;
}

TcpPort::~TcpPort()
{
    close();
}

bool TcpPort::open(const std::string &host, uint16_t port)
{
    close();

    addrinfo hints {};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    addrinfo *res = nullptr;

    std::string portStr = std::to_string(port);

    if (getaddrinfo(host.c_str(), portStr.c_str(), &hints, &res) != 0)
    {
        return false;
    }

    for (addrinfo *p = res; p != nullptr; p = p->ai_next)
    {
        int fd = ::socket(p->ai_family, p->ai_socktype, p->ai_protocol);

        if (fd < 0)
        {
            continue;
        }

        if (::connect(fd, p->ai_addr, p->ai_addrlen) == 0)
        {
            m_fd = fd;
            freeaddrinfo(res);
            return true;
        }

        ::close(fd);
    }

    freeaddrinfo(res);
    return false;
}

void TcpPort::close()
{
    if (m_fd >= 0)
    {
        ::shutdown(m_fd, SHUT_RDWR);
        ::close(m_fd);
        m_fd = -1;
    }
}

bool TcpPort::isOpen() const
{
    return m_fd >= 0;
}

bool TcpPort::writeAll(const uint8_t *data, size_t len)
{
    while (len > 0)
    {
        ssize_t n = ::send(m_fd, data, len, MSG_NOSIGNAL);

        if (n <= 0)
        {
            return false;
        }

        data += static_cast<size_t>(n);
        len -= static_cast<size_t>(n);
    }

    return true;
}

bool TcpPort::readExact(uint8_t *data, size_t len, int timeoutMs)
{
    auto start = std::chrono::steady_clock::now();
    size_t got = 0;

    while (got < len)
    {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

        int remaining = timeoutMs - static_cast<int>(elapsed);

        if (remaining <= 0)
        {
            return false;
        }

        pollfd pfd {};
        pfd.fd = m_fd;
        pfd.events = POLLIN;

        int pr = ::poll(&pfd, 1, remaining);

        if (pr <= 0)
        {
            return false;
        }

        ssize_t n = ::recv(m_fd, data + got, len - got, 0);

        if (n <= 0)
        {
            close();
            return false;
        }

        got += static_cast<size_t>(n);
    }

    return true;
}

bool TcpPort::readByte(uint8_t &byte, int timeoutMs)
{
    return readExact(&byte, 1, timeoutMs);
}