/**
   Copyright (c) 2015 Beckhoff Automation GmbH & Co. KG

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
 */

#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#include "Frame.h"
#include "wrap_socket.h"
#include <stdexcept>
#include <string>

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
};

struct UdpSocket : Socket {
    UdpSocket(IpV4 ip, uint16_t port);
};

#endif // UDPSOCKET_H
