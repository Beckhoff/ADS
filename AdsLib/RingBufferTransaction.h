// SPDX-License-Identifier: MIT

#pragma once

#include "RingBuffer.h"

#include <cassert>
#include <cstdint>

struct RingBufferTransaction {
	RingBufferTransaction(RingBuffer &target)
		: buffer(target)
		, write(target.write)
	{
	}

	size_t BytesFree() const
	{
		return (write < buffer.read) ?
			       buffer.read - write - 1 :
			       buffer.dataSize - 1 - (write - buffer.read);
	}

	size_t WriteChunk() const
	{
		return (write < buffer.read) ?
			       buffer.read - write - 1 :
			       buffer.data.get() + buffer.dataSize - write -
				       (buffer.data.get() == buffer.read);
	}

	void Write(size_t n)
	{
		assert(n <= BytesFree());
		write = buffer.Increment(write, n);
	}

	void Commit()
	{
		buffer.write = write;
	}

	RingBuffer &buffer;
	uint8_t *write;
};
