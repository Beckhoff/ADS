#ifndef WRAP_SOCKET_H
#define WRAP_SOCKET_H

#include <cstdint>

#if defined(__gnu_linux__) || defined(__APPLE__) || defined(__CYGWIN__)
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
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
inline int InitSocketLibrary(void)
{
    return 0;
}
#define NATIVE_SELECT(SOCK, READFDS, WRITEFDS, EXCEPTFDS, TIMEOUT) \
    ::select(SOCK, READFDS, WRITEFDS, EXCEPTFDS, TIMEOUT)
#else // defined(_WIN32) && !defined(__CYGWIN__)
#include <WinSock2.h>
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
#endif
#endif // WRAP_SOCKET_H
