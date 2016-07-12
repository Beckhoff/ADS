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

#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

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
#endif /* #ifndef _RING_BUFFER_H_ */
