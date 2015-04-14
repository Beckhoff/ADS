#ifndef QENDIAN_H
#define QENDIAN_H

#include <cstdint>

// WARNING code was never tested on a big endian machine!
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
inline T qFromLittleEndian(const uint8_t *value)
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
inline T qFromLittleEndian(const uint8_t *value)
{
    return *reinterpret_cast<const T*>(value);
}

inline uint16_t qToBigEndian(const uint16_t value)
{
    return ((value & 0xff) << 8) | ((value & 0xff00) >> 8);
}
#endif
#endif // QENDIAN_H
