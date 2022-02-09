// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
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
