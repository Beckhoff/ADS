/**
   Copyright (c) 2015 - 2016 Beckhoff Automation GmbH & Co. KG

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

#include "AdsDef.h"

#include <cstring>
#include <ostream>
#include <sstream>
#include <cstdlib>

bool operator<(const AmsAddr& lhs, const AmsAddr& rhs)
{
    if (memcmp(&lhs.netId, &rhs.netId, sizeof(lhs.netId))) {
        return lhs.netId < rhs.netId;
    }
    return lhs.port < rhs.port;
}

std::ostream& operator<<(std::ostream& os, const AmsNetId& netId)
{
    return os << std::dec << (int)netId.b[0] << '.' << (int)netId.b[1] << '.' << (int)netId.b[2] << '.' <<
           (int)netId.b[3] << '.' << (int)netId.b[4] << '.' << (int)netId.b[5];
}

AmsNetId::AmsNetId(uint32_t ipv4Addr)
{
    b[5] = 1;
    b[4] = 1;
    b[3] = (uint8_t)(ipv4Addr & 0x000000ff);
    b[2] = (uint8_t)((ipv4Addr & 0x0000ff00) >> 8);
    b[1] = (uint8_t)((ipv4Addr & 0x00ff0000) >> 16);
    b[0] = (uint8_t)((ipv4Addr & 0xff000000) >> 24);
}

AmsNetId::AmsNetId(uint8_t id_0, uint8_t id_1, uint8_t id_2, uint8_t id_3, uint8_t id_4, uint8_t id_5)
{
    b[5] = id_5;
    b[4] = id_4;
    b[3] = id_3;
    b[2] = id_2;
    b[1] = id_1;
    b[0] = id_0;
}

AmsNetId::AmsNetId(const std::string& addr)
{
    std::istringstream iss(addr);
    std::string s;
    size_t i = 0;

    while ((i < sizeof(b)) && std::getline(iss, s, '.')) {
        b[i] = atoi(s.c_str()) % 256;
        ++i;
    }

    if ((i != sizeof(b)) || std::getline(iss, s, '.')) {
        static const AmsNetId empty {};
        memcpy(b, empty.b, sizeof(b));
    }
}

bool AmsNetId::operator<(const AmsNetId& rhs) const
{
    for (unsigned int i = 0; i < sizeof(rhs.b); ++i) {
        if (b[i] != rhs.b[i]) {
            return b[i] < rhs.b[i];
        }
    }
    return false;
}

AmsNetId::operator bool() const
{
    static const AmsNetId empty {};
    return 0 != memcmp(b, &empty.b, sizeof(b));
}
