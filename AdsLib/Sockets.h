#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#include "Frame.h"
#include "wrap_socket.h"
#include <string>

struct IpV4
{
    IpV4(const std::string& addr);
    uint32_t htonl() const;

private:
    uint32_t value;
};

struct Socket
{
    Frame& read(Frame &frame) const;
    size_t write(const Frame &frame) const;

protected:
    int m_WSAInitialized;
    SOCKET m_Socket;
    sockaddr_in m_SockAddress;

    Socket(IpV4 ip, uint16_t port, int type);
    ~Socket();
    bool select(timeval *timeout) const;
};

struct TcpSocket : Socket
{
    TcpSocket(IpV4 ip, uint16_t port);
    uint32_t Connect() const;
};

struct UdpSocket : Socket
{
    UdpSocket(IpV4 ip, uint16_t port);
};

#endif // UDPSOCKET_H
