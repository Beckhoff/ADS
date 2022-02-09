// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
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
