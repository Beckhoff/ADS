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

#include <ostream>
#include <sstream>
#include <stdlib.h>
#include <string.h>

bool operator<(const AmsNetId& lhs, const AmsNetId& rhs)
{
    for (unsigned int i = 0; i < sizeof(rhs.b); ++i) {
        if (lhs.b[i] != rhs.b[i]) {
            return lhs.b[i] < rhs.b[i];
        }
    }
    return false;
}

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

AmsNetId make_AmsNetId(const std::string& addr)
{
    std::istringstream iss(addr);
    std::string s;
    size_t i = 0;
    AmsNetId id {};

    while ((i < sizeof(id.b)) && std::getline(iss, s, '.')) {
        id.b[i] = atoi(s.c_str()) % 256;
        ++i;
    }

    if ((i != sizeof(id.b)) || std::getline(iss, s, '.')) {
        memset(id.b, 0, sizeof(id.b));
    }
    return id;
}
