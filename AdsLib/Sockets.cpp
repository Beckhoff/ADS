// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#include "Sockets.h"
#include "Log.h"

#include <algorithm>
#include <climits>
#include <cstring>
#include <exception>
#include <sstream>
#include <system_error>

namespace bhf
{
namespace ads
{
AddressList GetListOfAddresses(const std::string& host, const std::string& service)
{
    struct addrinfo* results;

    if (getaddrinfo(host.c_str(), service.c_str(), nullptr, &results)) {
        throw std::runtime_error("Invalid or unknown host: " + host);
    }
    return AddressList { results, [](struct addrinfo* p) { freeaddrinfo(p); }};
}
}
}

static const struct addrinfo addrinfo = []() {
                                            struct addrinfo a;
                                            memset(&a, 0, sizeof(a));
                                            a.ai_family = AF_INET;
                                            a.ai_socktype = SOCK_STREAM;
                                            a.ai_protocol = IPPROTO_TCP;
                                            return a;
                                        } ();

uint32_t getIpv4(const std::string& addr)
{
    struct addrinfo* res;

    InitSocketLibrary();
    const auto status = getaddrinfo(addr.c_str(), nullptr, &addrinfo, &res);
    if (status) {
        throw std::runtime_error("Invalid IPv4 address or unknown hostname: " + addr);
    }

    const auto value = ((struct sockaddr_in*)res->ai_addr)->sin_addr.s_addr;
    freeaddrinfo(res);
    WSACleanup();
    return ntohl(value);
}

IpV4::IpV4(const std::string& addr)
    : value(getIpv4(addr))
{}

IpV4::IpV4(uint32_t __val)
    : value(__val)
{}

bool IpV4::operator<(const IpV4& ref) const
{
    return value < ref.value;
}

bool IpV4::operator==(const IpV4& ref) const
{
    return value == ref.value;
}

Socket::Socket(IpV4 ip, uint16_t port, int type)
    : m_WSAInitialized(!InitSocketLibrary()),
    m_Socket(socket(AF_INET, type, 0)),
    m_DestAddr(SOCK_DGRAM == type ? reinterpret_cast<const struct sockaddr*>(&m_SockAddress) : nullptr),
    m_DestAddrLen(m_DestAddr ? sizeof(m_SockAddress) : 0)
{
    if (INVALID_SOCKET == m_Socket) {
        throw std::system_error(WSAGetLastError(), std::system_category());
    }
    m_SockAddress.sin_family = AF_INET;
    m_SockAddress.sin_port = htons(port);
    m_SockAddress.sin_addr.s_addr = htonl(ip.value);
}

Socket::~Socket()
{
    Shutdown();
    closesocket(m_Socket);

    if (m_WSAInitialized) {
        WSACleanup();
    }
}

void Socket::Shutdown()
{
    shutdown(m_Socket, SHUT_RDWR);
}

size_t Socket::read(uint8_t* buffer, size_t maxBytes, timeval* timeout) const
{
    if (!Select(timeout)) {
        return 0;
    }

    maxBytes = static_cast<int>(std::min<size_t>(INT_MAX, maxBytes));
    const int bytesRead = recv(m_Socket, reinterpret_cast<char*>(buffer), maxBytes, 0);
    if (bytesRead > 0) {
        return bytesRead;
    }
    const auto lastError = WSAGetLastError();
    if ((0 == bytesRead) || (lastError == CONNECTION_CLOSED) || (lastError == CONNECTION_ABORTED)) {
        throw std::runtime_error("connection closed by remote");
    } else {
        LOG_ERROR("read frame failed with error: " << std::dec << std::strerror(lastError));
    }
    return 0;
}

Frame& Socket::read(Frame& frame, timeval* timeout) const
{
    const size_t bytesRead = read(frame.rawData(), frame.capacity(), timeout);
    if (bytesRead) {
        return frame.limit(bytesRead);
    }
    return frame.clear();
}

bool Socket::Select(timeval* timeout) const
{
    /* prepare socket set for select() */
    fd_set readSockets;
    FD_ZERO(&readSockets);
    FD_SET(m_Socket, &readSockets);

    /* wait for receive data */
    const int state = NATIVE_SELECT(m_Socket + 1, &readSockets, nullptr, nullptr, timeout);
    if (0 == state) {
        LOG_ERROR("select() timeout");
        throw TimeoutEx("select() timeout");
    }

    const auto lastError = WSAGetLastError();
    if (lastError == WSAENOTSOCK) {
        throw std::runtime_error("connection closed");
    }

    /* and check if socket was correct */
    if ((1 != state) || (!FD_ISSET(m_Socket, &readSockets))) {
        LOG_ERROR("something strange happen while waiting for socket in state: " <<
                  state << " with error: " << std::strerror(lastError));
        return false;
    }
    return true;
}

size_t Socket::write(const Frame& frame) const
{
    if (frame.size() > INT_MAX) {
        LOG_ERROR("frame length: " << frame.size() << " exceeds maximum length for sockets");
        return 0;
    }

    const int bufferLength = static_cast<int>(frame.size());
    const char* const buffer = reinterpret_cast<const char*>(frame.data());
    const int status = sendto(m_Socket, buffer, bufferLength, 0, m_DestAddr, m_DestAddrLen);

    if (SOCKET_ERROR == status) {
        LOG_ERROR("write frame failed with error: " << std::strerror(WSAGetLastError()));
        return 0;
    }
    return status;
}

TcpSocket::TcpSocket(const IpV4 ip, const uint16_t port)
    : Socket(ip, port, SOCK_STREAM)
{
    // AdsDll.lib seems to use TCP_NODELAY, we use it to be compatible
    const int enable = 0;
    if (setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, (const char*)&enable, sizeof(enable))) {
        LOG_WARN("Enabling TCP_NODELAY failed");
    }
}

uint32_t TcpSocket::Connect() const
{
    const uint32_t addr = ntohl(m_SockAddress.sin_addr.s_addr);

    if (::connect(m_Socket, reinterpret_cast<const sockaddr*>(&m_SockAddress), sizeof(m_SockAddress))) {
        LOG_ERROR("Connect TCP socket failed with: " << std::strerror(WSAGetLastError()));
        throw std::system_error(WSAGetLastError(), std::system_category());
    }

    struct sockaddr_in source;
    socklen_t len = sizeof(source);

    if (getsockname(m_Socket, reinterpret_cast<sockaddr*>(&source), &len)) {
        LOG_ERROR("Read local tcp/ip address failed");
        throw std::runtime_error("Read local tcp/ip address failed");
    }
    LOG_INFO("Connected to " << ((addr & 0xff000000) >> 24) << '.' << ((addr & 0xff0000) >> 16) << '.' <<
             ((addr & 0xff00) >> 8) << '.' << (addr & 0xff));

    return ntohl(source.sin_addr.s_addr);
}

bool TcpSocket::IsConnectedTo(const struct addrinfo* const targetAddresses) const
{
    for (auto rp = targetAddresses; rp; rp = rp->ai_next) {
        if (m_SockAddress.sin_family == rp->ai_family) {
            if (!memcmp(&m_SockAddress.sin_addr, &(((struct sockaddr_in*)(rp->ai_addr))->sin_addr),
                        sizeof(m_SockAddress.sin_addr)))
            {
                return true;
            }
        }
    }
    return false;
}

UdpSocket::UdpSocket(IpV4 ip, uint16_t port)
    : Socket(ip, port, SOCK_DGRAM)
{}
