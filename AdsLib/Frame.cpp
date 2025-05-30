// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2015 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#include "Frame.h"
#include "Log.h"

#include <algorithm>
#include <cstring>
#include <new>

Frame::Frame(size_t length, const void *data)
	: m_Data(new uint8_t[length])
{
	m_Size = m_Data ? length : 0;
	m_Pos = m_Data.get() + m_Size;
	m_OriginalSize = m_Size;

	if (m_Pos && data) {
		m_Pos -= length;
		memcpy(m_Pos, data, length);
	}
}

uint8_t Frame::operator[](size_t index) const
{
	return m_Pos[index];
}

size_t Frame::capacity() const
{
	return m_Size;
}

Frame &Frame::clear()
{
	return remove(size());
}

const uint8_t *Frame::data() const
{
	return m_Pos;
}

Frame &Frame::limit(size_t newSize)
{
	m_Size = std::min(m_Size, newSize);
	m_Pos = m_Data.get();
	return *this;
}

Frame &Frame::reset(const size_t newSize)
{
	if (newSize > m_OriginalSize) {
		try {
			std::unique_ptr<uint8_t[]> tmp{ new uint8_t[newSize] };
			m_OriginalSize = newSize;
			m_Data = std::move(tmp);
		} catch (const std::bad_alloc &) {
			LOG_WARN("Not enough memory to reset frame to "
				 << std::dec << newSize << " bytes");
		}
	}

	m_Size = m_OriginalSize;
	m_Pos = m_Data.get() + m_Size;
	return *this;
}

Frame &Frame::prepend(const void *const data, const size_t size)
{
	const size_t bytesFree = m_Pos - m_Data.get();
	if (size > bytesFree) {
		auto newData = new uint8_t[size + m_Size];

		m_Pos = newData + bytesFree + size;
		memcpy(m_Pos, m_Data.get() + bytesFree, m_Size - bytesFree);
		m_Data.reset(newData);
		m_Size += size;
		m_OriginalSize = m_Size;
		m_Pos = m_Data.get() + bytesFree;
	} else {
		m_Pos -= size;
	}
	memcpy(m_Pos, data, size);
	return *this;
}

uint8_t *Frame::rawData() const
{
	return m_Data.get();
}

Frame &Frame::remove(size_t numBytes)
{
	m_Pos = std::min<uint8_t *>(m_Pos + numBytes, m_Data.get() + m_Size);
	return *this;
}

size_t Frame::size() const
{
	return m_Size - (m_Pos - m_Data.get());
}
