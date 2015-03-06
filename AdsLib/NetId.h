#ifndef NETID_H
#define NETID_H

#include <cstdint>
#include <iosfwd>
#include <string>

#pragma pack (push, 1)
struct NetId
{
    uint8_t ams[6];

    NetId(uint32_t ipv4Addr = 0);
    NetId(const std::string &addr);
    NetId(const uint8_t* addr);
    operator bool () const;
	friend std::ostream& operator<<(std::ostream& os, const NetId& id);
private:
    void ipToAms(uint32_t ipv4Addr);
};
#pragma pack (pop)
#endif // NETID_H
