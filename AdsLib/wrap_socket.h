/**
   Copyright (c) 2015 - 2018 Beckhoff Automation GmbH & Co. KG

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

#pragma once

#include <stdint.h>

#if !(defined(_WIN32) && !defined(__CYGWIN__))
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>
typedef int SOCKET;
#define INVALID_SOCKET ((int)-1)
#define SOCKET_ERROR ((int)-1)
#define closesocket(X) ::close(X)
#define WSACleanup()
#define WSAGetLastError() errno
#define WSAENOTSOCK EBADF
#define CONNECTION_CLOSED ENOTCONN
#define CONNECTION_ABORTED ECONNABORTED
inline int InitSocketLibrary(void)
{
    return 0;
}
#define NATIVE_SELECT(SOCK, READFDS, WRITEFDS, EXCEPTFDS, TIMEOUT) \
    ::select(SOCK, READFDS, WRITEFDS, EXCEPTFDS, TIMEOUT)
#else // defined(_WIN32) && !defined(__CYGWIN__)
#define _WINSOCK_DEPRECATED_NO_WARNINGS 1
#include <winsock2.h>
#include <ws2tcpip.h>
inline int InitSocketLibrary(void)
{
    WSADATA wsaData;
    return WSAStartup(0x0202, &wsaData);
}

#define NATIVE_SELECT(SOCK, READFDS, WRITEFDS, EXCEPTFDS, TIMEOUT) \
    ::select(0, READFDS, WRITEFDS, EXCEPTFDS, TIMEOUT)

#define s_addr S_un.S_addr
typedef int socklen_t;
#define SHUT_RDWR SD_BOTH
#define CONNECTION_CLOSED WSAESHUTDOWN
#define CONNECTION_ABORTED WSAECONNABORTED
#endif
