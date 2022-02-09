// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace bhf
{
namespace ads
{
#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
template<class T>
inline T letoh(const void* buffer)
{
    const auto bytes = reinterpret_cast<const uint8_t*>(buffer);
    T result = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
        result += (bytes[i] << (8 * i));
    }
    return result;
}
#else
template<class T>
inline T letoh(const void* buffer)
{
    return *reinterpret_cast<const T*>(buffer);
}
#endif

template<class T>
inline T letoh(const T& value)
{
    return letoh<T>(reinterpret_cast<const uint8_t*>(&value));
}

template<class T>
T htole(const T value)
{
    return letoh(value);
}
}
}
