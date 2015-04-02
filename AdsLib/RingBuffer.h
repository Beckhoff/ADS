#ifndef _RING_BUFFER_H_
#define _RING_BUFFER_H_

#include <cassert>
#include <cstdint>

struct RingBuffer
{
	RingBuffer(size_t N = 4*1024*1024)
		: dataSize(N+1),
		data(new uint8_t[N + 1]),
		write(data.get()),
		read(data.get())
	{}

	size_t BytesFree() const
	{
		return (write < read) ? read - write - 1 : dataSize - 1 - (write - read);
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
			result += (*read << (8 * i));
			read = Increment(read, 1);
		}
		return result;
	}

	void Read(size_t n)
	{
		read = Increment(read, n);
	}

private:
	const size_t dataSize;
	const std::unique_ptr<uint8_t[]> data;

	inline uint8_t* Increment(const uint8_t *ptr, size_t n)
	{
		return data.get() + ((ptr - data.get() + n) % dataSize);
	}
public:
	uint8_t *write;
	const uint8_t *read;
};
#endif /* #ifndef _RING_BUFFER_H_ */