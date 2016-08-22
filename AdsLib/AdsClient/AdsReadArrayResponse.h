/**
    Class which is returned by the AdsClient ReadArray()-Operations.
 */

#pragma once

#include <stdexcept>

template<typename T, uint32_t count>
class AdsReadArrayResponse {
public:
    AdsReadArrayResponse<T, count>()
    {
        m_BytesRead = 0;
    }

    const T* Get() const
    {
        return &m_pArray;
    }

    T* GetPointer()
    {
        return &m_pArray[0];
    }

    const uint32_t GetBytesRead() const
    {
        return m_BytesRead;
    }

    void SetBytesRead(const uint32_t bytesRead)
    {
        m_BytesRead = bytesRead;
    }

    const uint32_t GetElementsRead() const
    {
        return m_BytesRead / sizeof(T);
    }

    T operator[](uint32_t index)
    {
        if (index < m_BytesRead / sizeof(T)) {
            return m_pArray[index];
        } else {
            throw std::out_of_range("The requested index is not inside the range of elements read from the server");
        }
    }

private:
    T m_pArray[count];
    uint32_t m_BytesRead;
};
