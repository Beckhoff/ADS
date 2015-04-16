#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#include "Frame.h"
#include "wrap_socket.h"
#include <string>

struct IpV4 {
    const uint32_t value;
    IpV4(const std::string& addr);
    bool operator<(const IpV4& ref) const;
    bool operator==(const IpV4& ref) const;
};

struct Socket {
    Frame& read(Frame& frame) const;
    size_t read(uint8_t* buffer, size_t maxBytes) const;
    size_t write(const Frame& frame) const;
    void Shutdown();

protected:
    int m_WSAInitialized;
    SOCKET m_Socket;
    sockaddr_in m_SockAddress;
    const sockaddr* const m_DestAddr;
    const size_t m_DestAddrLen;

    Socket(IpV4 ip, uint16_t port, int type);
    ~Socket();
    bool Select(timeval* timeout) const;
};

struct TcpSocket : Socket {
    TcpSocket(IpV4 ip, uint16_t port);
    uint32_t Connect() const;
};

struct UdpSocket : Socket {
    UdpSocket(IpV4 ip, uint16_t port);
};

#endif // UDPSOCKET_H
