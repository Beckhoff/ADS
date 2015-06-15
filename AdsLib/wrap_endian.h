/**
   Copyright (c) 2015 Beckhoff Automation GmbH & Co. KG

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

#ifndef QENDIAN_H
#define QENDIAN_H

#include <cstdint>

#if defined(__BYTE_ORDER__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__)
template<class T>
T qToLittleEndian(const T value);

template<>
inline uint16_t qToLittleEndian(const uint16_t value)
{
    return ((value & 0xff) << 8) | ((value & 0xff00) >> 8);
}

template<>
inline uint32_t qToLittleEndian(const uint32_t value)
{
    return ((value & 0xff) << 24) | ((value & 0xff00) << 8) | ((value & 0xff0000) >> 8) | ((value & 0xff000000) >> 24);
}

template<class T>
inline T qFromLittleEndian(const uint8_t* value)
{
    T result = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
        result += (value[i] << (8 * i));
    }
    return result;
}

template<class T>
inline T qToBigEndian(const T value)
{
    return value;
}
#else
template<class T>
inline T qToLittleEndian(const T value)
{
    return value;
}

template<class T>
inline T qFromLittleEndian(const uint8_t* value)
{
    return *reinterpret_cast<const T*>(value);
}

inline uint16_t qToBigEndian(const uint16_t value)
{
    return ((value & 0xff) << 8) | ((value & 0xff00) >> 8);
}
#endif
#endif // QENDIAN_H
