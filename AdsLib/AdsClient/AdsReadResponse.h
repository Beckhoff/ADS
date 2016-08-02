/**
    Class which is returned by the AdsClient Read()-Operations.
 */

#pragma once

#include <memory>

template<typename T>
class AdsReadResponse {
public:
    AdsReadResponse<T>()
    {
        m_pValue = std::shared_ptr<T>(new T);
    }

    const T& Get() const
    {
        return *m_pValue;
    }

    T* GetPointer() const
    {
        return m_pValue.get();
    }

    void SetBytesRead(const uint32_t bytesRead)
    {
        m_BytesRead = bytesRead;
    }

    const uint32_t GetBytesRead() const
    {
        return m_BytesRead;
    }

    operator T() const
    {
        return *m_pValue;
    }

private:
    std::shared_ptr<T> m_pValue;
    uint32_t m_BytesRead;
};
