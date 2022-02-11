// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include "Frame.h"
#include "wrap_socket.h"
#include <stdexcept>
#include <string>

namespace bhf
{
namespace ads
{
using AddressList = std::unique_ptr<struct addrinfo, void (*)(struct addrinfo*)>;
AddressList GetListOfAddresses(const std::string& host, const std::string& service = {});
}
}

struct IpV4 {
    const uint32_t value;
    IpV4(const std::string& addr);
    IpV4(uint32_t __val);
    bool operator<(const IpV4& ref) const;
    bool operator==(const IpV4& ref) const;
};

struct Socket {
    Frame& read(Frame& frame, timeval* timeout) const;
    size_t read(uint8_t* buffer, size_t maxBytes, timeval* timeout) const;
    size_t write(const Frame& frame) const;
    void Shutdown();

    struct TimeoutEx : std::runtime_error {
        TimeoutEx(const char* _Message) : std::runtime_error(_Message)
        {}
    };

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

    /**
     * Confirm if this TcpSocket is connected to one of the target addresses.
     * @param[in] targetAddresses pointer to a previously allocated list of
     *           "struct addrinfo" returned by getaddrinfo(3).
     * @return true, this connection can be used to reach one of the targetAddresses.
     */
    bool IsConnectedTo(const struct addrinfo* targetAddresses) const;
};

struct UdpSocket : Socket {
    UdpSocket(IpV4 ip, uint16_t port);
};
