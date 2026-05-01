// SPDX-License-Identifier: MIT
#pragma once

#include "Frame.h"
#include "SecureAdsConfig.h"
#include "wrap_socket.h"

#include <openssl/ssl.h>
#include <stdexcept>
#include <string>
#include <vector>

struct TlsSocket {
	TlsSocket(const struct addrinfo *host,
		  const bhf::ads::SecureAdsConfig &config);
	~TlsSocket();

	TlsSocket(const TlsSocket &) = delete;
	TlsSocket &operator=(const TlsSocket &) = delete;

	size_t read(uint8_t *buffer, size_t maxBytes, timeval *timeout) const;
	size_t write(const Frame &frame) const;
	void write_raw(const uint8_t *data, size_t length) const;
	void read_raw(uint8_t *data, size_t length) const;
	void Shutdown();
	bool IsConnectedTo(const struct addrinfo *targetAddresses) const;
	uint32_t Connect() const;

	struct TimeoutEx : std::runtime_error {
		TimeoutEx(const char *msg)
			: std::runtime_error(msg)
		{
		}
	};

    private:
	SOCKET m_Fd{ INVALID_SOCKET };
	SSL_CTX *m_Ctx{ nullptr };
	SSL *m_Ssl{ nullptr };
	sockaddr_storage m_SockAddress{};
	socklen_t m_AddrLen{ 0 };

	std::vector<uint8_t> m_DerivedPsk;
	std::string m_PskIdentity;
	bool m_IsPsk{ false };

	void InitCtx(const bhf::ads::SecureAdsConfig &config);
	bool Select(timeval *timeout) const;

	static int pskExIndex();
	static unsigned int pskClientCallback(SSL *ssl, const char *hint,
					      char *identity,
					      unsigned int maxIdLen,
					      unsigned char *psk,
					      unsigned int maxPskLen);
	static std::vector<uint8_t> derivePsk(const std::string &identity,
					      const std::string &password);
};
