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
        m_pArray = std::shared_ptr<T>(new T[count], array_deleter<T>());
        m_BytesRead = 0;
    }

    const T* Get() const
    {
        return m_pArray.get();
    }

    T* GetPointer() const
    {
        return m_pArray.get();
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
            return m_pArray.get()[index];
        } else {
            throw std::out_of_range("The requested index is not inside the range of elements read from the server");
        }
    }

private:
    std::shared_ptr<T> m_pArray;
    uint32_t m_BytesRead;

    template<typename T1>
    class array_deleter {
public:
        void operator()(T1 const* p)
        {
            delete[] p;
        }
    };
};
