#include "NetId.h"

#include <sstream>

NetId::NetId(const uint32_t ipv4Addr)
{
    ipToAms(ipv4Addr);
}

NetId::NetId(const std::string &addr)
{
    std::istringstream iss(addr);
    std::string s;
    size_t i = 0;

    while ((i < sizeof(ams)) && std::getline(iss, s, '.')) {
        ams[i] = atoi(s.c_str()) % 256;
        ++i;
    }

    if (i != sizeof(ams)) {
        ipToAms(0);
    }
}

NetId::NetId(const uint8_t* addr)
{
    memcpy(ams, addr, sizeof(ams));
}

void NetId::ipToAms(uint32_t ipv4Addr)
{
    ams[5] = 1;
    ams[4] = 1;
    ams[3] = (uint8_t)(ipv4Addr & 0x000000ff);
    ams[2] = (uint8_t)((ipv4Addr & 0x0000ff00) >> 8);
    ams[1] = (uint8_t)((ipv4Addr & 0x00ff0000) >> 16);
    ams[0] = (uint8_t)((ipv4Addr & 0xff000000) >> 24);
}

NetId::operator bool() const
{
    static const NetId empty{};
    return 0 != memcmp(this, &empty, sizeof(*this));
}

std::ostream& operator<<(std::ostream& os, const NetId& id)
{
	return os << (int)id.ams[0] << '.' << (int)id.ams[1] << '.' << (int)id.ams[2] << '.'
		<< (int)id.ams[3] << '.' << (int)id.ams[4] << '.' << (int)id.ams[5];
}
