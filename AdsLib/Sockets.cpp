#include "Sockets.h"
#include <algorithm>
#include <climits>
#include <sstream>
#include <system_error>

IpV4::IpV4(const std::string& addr)
{
    std::istringstream iss(addr);
    std::string s;
    int shift = 32;
    uint32_t result = 0;

    while (shift && std::getline(iss, s, '.')) {
        shift -= 8;
        result |= (atoi(s.c_str()) % 256) << shift;
    }
    value = shift ? 0 : result;
}

uint32_t IpV4::toNetworkOrder() const
{
    return htonl(value);
}

Socket::Socket(IpV4 ip, uint16_t port, int type)
    : m_WSAInitialized(!InitSocketLibrary()),
      m_Socket(socket(AF_INET, type, 0))
{
    if (INVALID_SOCKET == m_Socket) {
        throw std::system_error(WSAGetLastError(), std::system_category());
    }
    m_SockAddress.sin_family = AF_INET;
    m_SockAddress.sin_port = htons(port);
    m_SockAddress.sin_addr.s_addr = ip.toNetworkOrder();
}

Socket::~Socket()
{
    if (INVALID_SOCKET != m_Socket) {
        closesocket(m_Socket);
    }

    if (m_WSAInitialized) {
        WSACleanup();
    }
}

Frame& Socket::read(Frame &frame) const
{
	timeval timeout = { 1, 0 };
	if (!select(&timeout)) {
        return frame.clear();
    }

    const int maxBytes = static_cast<int>(std::min<size_t>(INT_MAX, frame.capacity()));
    const int bytesRead = recv(m_Socket, frame.rawData(), maxBytes, 0);
    if (bytesRead > 0) {
        return frame.limit(bytesRead);
    }
    if (0 == bytesRead) {
//        LOG_ERROR("connection closed by remote");
    } else {
//        LOG_ERROR("read frame failed with error: " << WSAGetLastError());
    }
    return frame.clear();
}

bool Socket::select(timeval *timeout) const
{
    /* prepare socket set for select() */
    fd_set readSockets;
    FD_ZERO(&readSockets);
    FD_SET(m_Socket, &readSockets);

    /* wait for receive data */
    const int state = NATIVE_SELECT(m_Socket + 1, &readSockets, NULL, NULL, timeout);
    if(0 == state) {
//        LOG_ERROR("select() timeout");
        return false;
    }

    /* and check if socket was correct */
    if((1 != state) || (!FD_ISSET(m_Socket, &readSockets))) {
//        LOG_ERROR("something strange happen while waiting for socket...");
        return false;
    }
    return true;
}

size_t Socket::write(const Frame &frame) const
{
    if (frame.size() > INT_MAX) {
//        LOG_ERROR("frame length: " << frame.size() << " exceeds maximum length for sockets");
        return 0;
    }

    const int bufferLength = static_cast<int>(frame.size());
    const char *const buffer = reinterpret_cast<const char*>(frame.data());
    const struct sockaddr *const addr = reinterpret_cast<const struct sockaddr*>(&m_SockAddress);

    const int  status = sendto(m_Socket, buffer, bufferLength, 0, addr, sizeof(m_SockAddress));
    if (SOCKET_ERROR == status) {
//        LOG_ERROR("read frame failed with error: " << WSAGetLastError());
        return 0;
    }
    return status;
}

TcpSocket::TcpSocket(const IpV4 ip, const uint16_t port)
    : Socket(ip, port, SOCK_STREAM)
{
}

uint32_t TcpSocket::Connect() const
{
//	const uint32_t addr = ntohl(m_SockAddress.sin_addr.s_addr);
//	LOG_INFO("Connecting to " << ((addr & 0xff000000) >> 24) << '.' << ((addr & 0xff0000) >> 16) << '.' << ((addr & 0xff00) >> 8) << '.' << (addr & 0xff));

    if (::connect(m_Socket, reinterpret_cast<const sockaddr *>(&m_SockAddress), sizeof(m_SockAddress))) {
//        LOG_ERROR("Connect TCP socket failed with: " << WSAGetLastError());
        return 0;
    }

    struct sockaddr_in source;
    socklen_t len = sizeof(source);

    if (getsockname(m_Socket, reinterpret_cast<sockaddr*>(&source), &len)) {
//        LOG_ERROR("Read local tcp/ip address failed");
        return 0;
    }
    return ntohl(source.sin_addr.s_addr);
}

UdpSocket::UdpSocket(IpV4 ip, uint16_t port)
    : Socket(ip, port, SOCK_DGRAM)
{
}
