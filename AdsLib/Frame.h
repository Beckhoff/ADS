// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include "wrap_endian.h"
#include <memory>

struct Frame {
    /**
     * @brief Frame
     * @param length number of bytes preallocated in the internale buffer
     * @param data, if not null this frame will be initialized with <lenght> number of bytes from <data>
     */
    Frame(size_t length, const void* data = nullptr);

    /**
     * @brief operator []
     * bytewise access to the frame bytes
     * @param index should be within the frames bounds!
     * @return
     */
    uint8_t operator[](size_t index) const;

    /**
     * @brief capacity
     * @return number of bytes, which are accessable by rawData()
     */
    size_t capacity() const;

    /**
     * @brief clear
     * Reset internal buffer to an empty frame
     * @return reference to self
     */
    Frame& clear();

    /**
     * @brief limit
     * If the frame is reused as a response buffer, call limit() to lock the frames state
     * and limit its size to the length of the response
     * @param newSize length of the response
     * @return reference to self
     */
    Frame& limit(size_t newSize);

    /**
     * @brief reset
     * Prepare the frame to be reused as a response buffer
     * @param newSize expected size of the response
     * @return reference to self
     */
    Frame& reset(size_t newSize = 4096);

    /**
     * @brief data
     * @return a pointer to the beginning of the frame
     */
    const uint8_t* data() const;

    /**
     * remove sizeof(T) bytes from the beginning of the frame and return them
     * interpreted as T. If frame is to short to fit a T, we silently return
     * a default initialized T.
     */
    template<typename T> T pop()
    {
        T value {};
        if (sizeof(value) <= capacity()) {
            value = *reinterpret_cast<T*>(m_Pos);
        }
        remove(sizeof(T));
        return value;
    }

    /**
     * remove sizeof(T) bytes from the beginning of the frame and return them
     * interpreted as T in little endian byte order (result is in host order).
     */
    template<typename T> T pop_letoh()
    {
        return bhf::ads::letoh(pop<T>());
    }

    /**
     * @brief prepend
     * prepend <data> in front of the frame
     * @param data
     * @param size number of bytes in <data>
     * @return reference to self
     */
    Frame& prepend(const void* const data, const size_t size);

    /**
     * prepend a header of type <T>
     */
    template<class T> Frame& prepend(const T& header)
    {
        return prepend(&header, sizeof(T));
    }

    /**
     * @brief rawData
     * Reuse the frame as response buffer, use with care!
     * @return a raw pointer to the beginning of the internal allocated buffer
     */
    uint8_t* rawData() const;

    /**
     * @brief remove
     * remove <numBytest> from the beginning of the frame
     * @param numBytes
     * @return reference to self
     */
    Frame& remove(size_t numBytes);

    /**
     * remove header of type <T> from the beginning of the frame
     */
    template<class T> T remove()
    {
        const auto p = m_Pos;
        remove(sizeof(T));
        return T(p);
    }

    /**
     * @brief size
     * @return length of the frame in bytes
     */
    size_t size() const;

private:
    std::unique_ptr<uint8_t[]> m_Data;
    uint8_t* m_Pos;
    size_t m_Size;
    size_t m_OriginalSize;
};
