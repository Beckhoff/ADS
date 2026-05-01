// SPDX-License-Identifier: MIT
#pragma once

#include <string>

namespace bhf
{
namespace ads
{

struct SecureAdsConfig {
	enum class Mode { SSC, SCA, PSK };

	Mode mode = Mode::SCA;
	std::string certPath; ///< SSC/SCA: PEM path to client certificate
	std::string keyPath; ///< SSC/SCA: PEM path to client private key
	std::string caPath; ///< SCA: PEM path to CA certificate
	std::string username; ///< SSC: username for first-time route registration
	std::string
		password; ///< SSC: first-time registration password; PSK: password for key derivation
	std::string
		pskIdentity; ///< PSK: identity string; key = SHA-256(UPPER(identity) + password)
};

}
}
