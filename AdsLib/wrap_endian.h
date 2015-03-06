#ifndef QENDIAN_H
#define QENDIAN_H

#include <stdint.h>

// WARNING code was never tested on a big endian machine!
#if defined(_WIN32) || defined(__ORDER_LITTLE_ENDIAN__)
template<class T>
inline T qToLittleEndian(const T value)
{
    return value;
}

template<class T>
inline T qFromLittleEndian(const uint8_t *value)
{
    T result = 0;
    for (int i = 0; i < sizeof(T); ++i) {
        result += (value[i] << (8 * i));
    }
    return result;
}

inline uint16_t qToBigEndian(const uint16_t value)
{
    return ((value & 0xff) << 8) | ((value & 0xff00) >> 8);
}
#endif
#endif // QENDIAN_H
