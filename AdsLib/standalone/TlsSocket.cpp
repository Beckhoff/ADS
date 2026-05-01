// SPDX-License-Identifier: MIT
#include "TlsSocket.h"
#include "Log.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <system_error>

#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

#include <cctype>

static std::string opensslError()
{
	const unsigned long e = ERR_get_error();
	if (!e) {
		return "unknown OpenSSL error";
	}
	char buf[256];
	ERR_error_string_n(e, buf, sizeof(buf));
	return std::string(buf);
}

int TlsSocket::pskExIndex()
{
	static int idx =
		SSL_get_ex_new_index(0, nullptr, nullptr, nullptr, nullptr);
	return idx;
}

std::vector<uint8_t> TlsSocket::derivePsk(const std::string &identity,
					  const std::string &password)
{
	std::string upper;
	upper.reserve(identity.size());
	for (unsigned char c : identity) {
		upper += static_cast<char>(toupper(c));
	}
	const std::string data = upper + password;
	std::vector<uint8_t> digest(SHA256_DIGEST_LENGTH);
	SHA256(reinterpret_cast<const uint8_t *>(data.data()), data.size(),
	       digest.data());
	return digest;
}

unsigned int TlsSocket::pskClientCallback(SSL *ssl, const char * /*hint*/,
					  char *identity, unsigned int maxIdLen,
					  unsigned char *psk,
					  unsigned int maxPskLen)
{
	auto *sock =
		static_cast<TlsSocket *>(SSL_get_ex_data(ssl, pskExIndex()));
	if (!sock || sock->m_DerivedPsk.empty()) {
		return 0;
	}

	const size_t idLen =
		std::min(sock->m_PskIdentity.size(), size_t(maxIdLen - 1));
	memcpy(identity, sock->m_PskIdentity.c_str(), idLen);
	identity[idLen] = '\0';

	const size_t keyLen =
		std::min(sock->m_DerivedPsk.size(), size_t(maxPskLen));
	memcpy(psk, sock->m_DerivedPsk.data(), keyLen);
	return static_cast<unsigned int>(keyLen);
}

static int sscVerifyCallback(int /*preverify_ok*/, X509_STORE_CTX *ctx)
{
	switch (X509_STORE_CTX_get_error(ctx)) {
	case X509_V_OK:
	case X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT:
	case X509_V_ERR_SELF_SIGNED_CERT_IN_CHAIN:
	case X509_V_ERR_UNABLE_TO_GET_ISSUER_CERT_LOCALLY:
	case X509_V_ERR_UNABLE_TO_VERIFY_LEAF_SIGNATURE:
		return 1;
	default:
		return 0;
	}
}

TlsSocket::TlsSocket(const struct addrinfo *host,
		     const bhf::ads::SecureAdsConfig &config)
{
	for (const auto *rp = host; rp; rp = rp->ai_next) {
		m_Fd = socket(rp->ai_family, SOCK_STREAM, 0);
		if (INVALID_SOCKET == m_Fd) {
			continue;
		}
		if (0 == connect(m_Fd, rp->ai_addr,
				 static_cast<socklen_t>(rp->ai_addrlen))) {
			memcpy(&m_SockAddress, rp->ai_addr,
			       std::min<size_t>(sizeof(m_SockAddress),
						rp->ai_addrlen));
			m_AddrLen = static_cast<socklen_t>(rp->ai_addrlen);
			break;
		}
		closesocket(m_Fd);
		m_Fd = INVALID_SOCKET;
	}

	if (INVALID_SOCKET == m_Fd) {
		throw std::system_error(WSAGetLastError(),
					std::system_category(),
					"TlsSocket: TCP connect failed");
	}

	const int nodelay = 0;
	setsockopt(m_Fd, IPPROTO_TCP, TCP_NODELAY,
		   reinterpret_cast<const char *>(&nodelay), sizeof(nodelay));

	InitCtx(config);

	m_Ssl = SSL_new(m_Ctx);
	if (!m_Ssl) {
		throw std::runtime_error("SSL_new failed: " + opensslError());
	}
	SSL_set_fd(m_Ssl, static_cast<int>(m_Fd));

	if (m_IsPsk) {
		SSL_set_ex_data(m_Ssl, pskExIndex(), this);
		SSL_set_psk_client_callback(m_Ssl, pskClientCallback);
	}

	if (SSL_connect(m_Ssl) != 1) {
		throw std::runtime_error("TLS handshake failed: " +
					 opensslError());
	}
}

void TlsSocket::InitCtx(const bhf::ads::SecureAdsConfig &config)
{
	m_Ctx = SSL_CTX_new(TLS_client_method());
	if (!m_Ctx) {
		throw std::runtime_error("SSL_CTX_new failed: " +
					 opensslError());
	}

	SSL_CTX_set_min_proto_version(m_Ctx, TLS1_2_VERSION);
	SSL_CTX_set_max_proto_version(m_Ctx, TLS1_2_VERSION);

	if (config.mode == bhf::ads::SecureAdsConfig::Mode::PSK) {
		m_PskIdentity = config.pskIdentity;
		m_DerivedPsk = derivePsk(config.pskIdentity, config.password);
		m_IsPsk = true;

		if (SSL_CTX_set_cipher_list(m_Ctx, "PSK-AES256-CBC-SHA384:"
						   "PSK-AES128-CBC-SHA256:"
						   "PSK-AES256-CBC-SHA:"
						   "PSK-AES128-CBC-SHA") != 1) {
			throw std::runtime_error(
				"Failed to set PSK cipher list: " +
				opensslError());
		}
		SSL_CTX_set_options(m_Ctx, SSL_OP_NO_ENCRYPT_THEN_MAC |
						   SSL_OP_NO_TICKET);
#ifdef SSL_OP_NO_EXTENDED_MASTER_SECRET
		SSL_CTX_set_options(m_Ctx, SSL_OP_NO_EXTENDED_MASTER_SECRET);
#endif
		SSL_CTX_set_verify(m_Ctx, SSL_VERIFY_NONE, nullptr);
		return;
	}

	if (SSL_CTX_use_certificate_file(m_Ctx, config.certPath.c_str(),
					 SSL_FILETYPE_PEM) != 1) {
		throw std::runtime_error("Failed to load cert '" +
					 config.certPath +
					 "': " + opensslError());
	}
	if (SSL_CTX_use_PrivateKey_file(m_Ctx, config.keyPath.c_str(),
					SSL_FILETYPE_PEM) != 1) {
		throw std::runtime_error("Failed to load key '" +
					 config.keyPath +
					 "': " + opensslError());
	}
	if (!SSL_CTX_check_private_key(m_Ctx)) {
		throw std::runtime_error("Cert/key mismatch: " +
					 opensslError());
	}

	if (config.mode == bhf::ads::SecureAdsConfig::Mode::SCA) {
		if (!config.caPath.empty() &&
		    SSL_CTX_load_verify_locations(m_Ctx, config.caPath.c_str(),
						  nullptr) != 1) {
			throw std::runtime_error("Failed to load CA '" +
						 config.caPath +
						 "': " + opensslError());
		}
		SSL_CTX_set_verify(m_Ctx,
				   SSL_VERIFY_PEER |
					   SSL_VERIFY_FAIL_IF_NO_PEER_CERT,
				   nullptr);
	} else {
		SSL_CTX_set_verify(m_Ctx, SSL_VERIFY_PEER, sscVerifyCallback);
	}
}

TlsSocket::~TlsSocket()
{
	Shutdown();
	if (m_Ssl) {
		SSL_free(m_Ssl);
		m_Ssl = nullptr;
	}
	if (m_Ctx) {
		SSL_CTX_free(m_Ctx);
		m_Ctx = nullptr;
	}
	if (INVALID_SOCKET != m_Fd) {
		closesocket(m_Fd);
		m_Fd = INVALID_SOCKET;
	}
}

void TlsSocket::Shutdown()
{
	if (m_Ssl) {
		SSL_shutdown(m_Ssl);
	}
}

bool TlsSocket::Select(timeval *timeout) const
{
	fd_set readFds;
	FD_ZERO(&readFds);
	FD_SET(m_Fd, &readFds);

	const int state = NATIVE_SELECT(static_cast<int>(m_Fd) + 1, &readFds,
					nullptr, nullptr, timeout);
	if (0 == state) {
		LOG_ERROR("TlsSocket select() timeout");
		throw TimeoutEx("TlsSocket select() timeout");
	}
	if (state < 0 || !FD_ISSET(m_Fd, &readFds)) {
		LOG_ERROR("TlsSocket select() error: "
			  << std::strerror(WSAGetLastError()));
		return false;
	}
	return true;
}

size_t TlsSocket::read(uint8_t *buffer, size_t maxBytes, timeval *timeout) const
{
	if (SSL_pending(m_Ssl) == 0) {
		if (!Select(timeout)) {
			return 0;
		}
	}

	const int toRead = static_cast<int>(
		std::min<size_t>(maxBytes, std::numeric_limits<int>::max()));
	const int n = SSL_read(m_Ssl, reinterpret_cast<char *>(buffer), toRead);
	if (n > 0) {
		return static_cast<size_t>(n);
	}
	const int err = SSL_get_error(m_Ssl, n);
	if (err == SSL_ERROR_ZERO_RETURN || err == SSL_ERROR_SYSCALL) {
		throw std::runtime_error("TLS connection closed by remote");
	}
	LOG_ERROR("SSL_read failed: " << opensslError());
	return 0;
}

size_t TlsSocket::write(const Frame &frame) const
{
	const int len = static_cast<int>(frame.size());
	const int n = SSL_write(
		m_Ssl, reinterpret_cast<const char *>(frame.data()), len);
	if (n <= 0) {
		LOG_ERROR("SSL_write failed: " << opensslError());
		return 0;
	}
	return static_cast<size_t>(n);
}

void TlsSocket::write_raw(const uint8_t *data, size_t length) const
{
	size_t sent = 0;
	while (sent < length) {
		const int toSend = static_cast<int>(std::min<size_t>(
			length - sent, std::numeric_limits<int>::max()));
		const int n = SSL_write(
			m_Ssl, reinterpret_cast<const char *>(data + sent),
			toSend);
		if (n <= 0) {
			throw std::runtime_error("TLS write_raw failed: " +
						 opensslError());
		}
		sent += static_cast<size_t>(n);
	}
}

void TlsSocket::read_raw(uint8_t *data, size_t length) const
{
	size_t received = 0;
	while (received < length) {
		timeval timeout{ 5, 0 };
		const size_t n =
			read(data + received, length - received, &timeout);
		if (0 == n) {
			throw std::runtime_error(
				"TLS read_raw: connection closed unexpectedly");
		}
		received += n;
	}
}

uint32_t TlsSocket::Connect() const
{
	sockaddr_storage source;
	socklen_t len = sizeof(source);

	if (getsockname(m_Fd, reinterpret_cast<sockaddr *>(&source), &len)) {
		LOG_ERROR("TlsSocket: getsockname failed");
		return 0;
	}
	if (source.ss_family == AF_INET) {
		return ntohl(reinterpret_cast<const sockaddr_in *>(&source)
				     ->sin_addr.s_addr);
	}
	return 0xffffffff;
}

bool TlsSocket::IsConnectedTo(const struct addrinfo *targetAddresses) const
{
	for (const auto *rp = targetAddresses; rp; rp = rp->ai_next) {
		if (m_SockAddress.ss_family == rp->ai_family &&
		    !memcmp(&m_SockAddress, rp->ai_addr,
			    std::min<size_t>(sizeof(m_SockAddress),
					     rp->ai_addrlen))) {
			return true;
		}
	}
	return false;
}
