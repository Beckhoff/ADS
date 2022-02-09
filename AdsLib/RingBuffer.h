// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include <cassert>
#include <cstdint>
#include <memory>

struct RingBuffer {
    RingBuffer(size_t N)
        : dataSize(N + 1),
        data(new uint8_t[N + 1]),
        write(data.get()),
        read(data.get())
    {}

    size_t BytesFree() const
    {
        return (write < read) ? read - write - 1 : dataSize - 1 - (write - read);
    }

    size_t BytesAvailable() const
    {
        return dataSize - BytesFree() - 1;
    }

    size_t WriteChunk() const
    {
        return (write < read) ? read - write - 1 : data.get() + dataSize - write - (data.get() == read);
    }

    void Write(size_t n)
    {
        assert(n <= BytesFree());
        write = Increment(write, n);
    }

    template<class T> T ReadFromLittleEndian()
    {
        T result = 0;
        for (size_t i = 0; i < sizeof(T); ++i) {
            result += (((T)(*read)) << (8 * i));
            read = Increment(read, 1);
        }
        return result;
    }

    void Read(size_t n)
    {
        assert(n <= BytesAvailable());
        read = Increment(read, n);
    }

private:
    const size_t dataSize;
    const std::unique_ptr<uint8_t[]> data;

    inline uint8_t* Increment(const uint8_t* ptr, size_t n)
    {
        return data.get() + ((ptr - data.get() + n) % dataSize);
    }
public:
    uint8_t* write;
    const uint8_t* read;
};
