#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#include <cassert>
#include <cstdint>

template<size_t N>
struct RingBuffer
{
	RingBuffer()
		: write(data),
		read(data)
	{}

	size_t BytesFree() const
	{
		return (write < read) ? read - write - 1 : sizeof(data) - 1 - (write - read);
	}

	size_t WriteChunk() const
	{
		return (write < read) ? read - write - 1 : data + sizeof(data) - write - (data == read);
	}

	void Write(size_t n)
	{
		assert(n <= BytesFree());
		write = Increment(write, n);
	}

	void Read(size_t n)
	{
		assert(n <= BytesAvailable());
		read = Increment(read, n);
	}

	uint8_t *write;
	const uint8_t *read;
private:
	uint8_t data[N + 1];

	inline uint8_t* Increment(const uint8_t *ptr, size_t n)
	{
		return data + (((size_t)ptr + n) % sizeof(data));
	}
};
#endif /* #ifndef _RING_BUFFER_H_ */