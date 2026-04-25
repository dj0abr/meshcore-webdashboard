#include "SerialPort.h"

#include <chrono>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

SerialPort::SerialPort()
{
    m_fd = -1;
}

SerialPort::~SerialPort()
{
    close();
}

bool SerialPort::open(const std::string &device, int baud)
{
    close();

    m_fd = ::open(device.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (m_fd < 0)
    {
        return false;
    }

    if (!configure(baud))
    {
        close();
        return false;
    }

    tcflush(m_fd, TCIOFLUSH);
    return true;
}

void SerialPort::close()
{
    if (m_fd >= 0)
    {
        ::close(m_fd);
        m_fd = -1;
    }
}

bool SerialPort::isOpen() const
{
    return m_fd >= 0;
}

int SerialPort::fd() const
{
    return m_fd;
}

bool SerialPort::configure(int baud)
{
    termios tty {};
    if (tcgetattr(m_fd, &tty) != 0)
    {
        return false;
    }

    speed_t spd = B115200;
    (void)baud; // optional später

    cfsetospeed(&tty, spd);
    cfsetispeed(&tty, spd);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag = 0;
    tty.c_lflag = 0;
    tty.c_oflag = 0;

    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 2;

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(m_fd, TCSANOW, &tty) != 0)
    {
        return false;
    }

    return true;
}

bool SerialPort::writeAll(const uint8_t *data, size_t len)
{
    while (len > 0)
    {
        ssize_t n = ::write(m_fd, data, len);
        if (n < 0)
        {
            return false;
        }

        data += static_cast<size_t>(n);
        len  -= static_cast<size_t>(n);
    }

    return true;
}

bool SerialPort::readExact(uint8_t *data, size_t len, int timeoutMs)
{
    auto start = std::chrono::steady_clock::now();
    size_t got = 0;

    while (got < len)
    {
        ssize_t n = ::read(m_fd, data + got, len - got);

        if (n > 0)
        {
            got += static_cast<size_t>(n);
            continue;
        }

        ::usleep(5 * 1000);

        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();

        if (elapsed > timeoutMs)
        {
            return false;
        }
    }

    return true;
}

bool SerialPort::readByte(uint8_t &byte, int timeoutMs)
{
    return readExact(&byte, 1, timeoutMs);
}

