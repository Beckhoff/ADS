#pragma once

#include "AdsHandle.h"

template<typename T>
struct AdsVariable {
    AdsVariable(const AmsAddr address, const std::string& symbolName, const long localPort)
        : m_RemoteAddr(address),
        m_LocalPort(localPort),
        m_IndexGroup(ADSIGRP_SYM_VALBYHND),
        m_Handle(address, localPort, symbolName)
    {}

    AdsVariable(const AmsAddr address, const uint32_t group, const uint32_t offset, const long localPort)
        : m_RemoteAddr(address),
        m_LocalPort(localPort),
        m_IndexGroup(group),
        m_Handle(offset)
    {}

    void Read(const size_t size, void* data) const
    {
        uint32_t bytesRead = 0;
        auto error = AdsSyncReadReqEx2(m_LocalPort,
                                       &m_RemoteAddr,
                                       m_IndexGroup,
                                       m_Handle,
                                       size,
                                       data,
                                       &bytesRead);

        if (error || (size != bytesRead)) {
            throw AdsException(error);
        }
    }

    void Write(const size_t size, const void* data) const
    {
        auto error = AdsSyncWriteReqEx(m_LocalPort,
                                       &m_RemoteAddr,
                                       m_IndexGroup,
                                       m_Handle,
                                       size,
                                       data);

        if (error) {
            throw AdsException(error);
        }
    }

    operator T() const
    {
        T buffer;
        Read(sizeof(buffer), &buffer);
        return buffer;
    }

    void operator=(const T& value) const
    {
        Write(sizeof(T), &value);
    }

    template<typename U, size_t N>
    operator std::array<U, N>() const
    {
        std::array<U, N> buffer;
        Read(sizeof(U) * N, buffer.data());
        return buffer;
    }

    template<typename U, size_t N>
    void operator=(const std::array<U, N>& value) const
    {
        Write(sizeof(U) * N, value.data());
    }
private:
    const AmsAddr m_RemoteAddr;
    const long m_LocalPort;
    const uint32_t m_IndexGroup;
    AdsHandle m_Handle;
};
