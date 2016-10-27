#pragma once

#include "AdsRoute.h"
#include "AdsHandle.h"

template<typename T>
struct AdsVariable {
    AdsVariable(const AdsRoute route, const std::string& symbolName)
        : m_Route(route),
        m_AmsAddr(route.m_SymbolPort),
        m_IndexGroup(ADSIGRP_SYM_VALBYHND),
        m_Handle(route.m_SymbolPort, route.GetLocalPort(), symbolName)
    {}

    AdsVariable(const AdsRoute route, const uint32_t group, const uint32_t offset)
        : m_Route(route),
        m_AmsAddr(route.m_TaskPort),
        m_IndexGroup(group),
        m_Handle(offset)
    {}

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

    void Read(const size_t size, void* data) const
    {
        uint32_t bytesRead = 0;
        auto error = AdsSyncReadReqEx2(m_Route.GetLocalPort(),
                                       &m_AmsAddr,
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
        auto error = AdsSyncWriteReqEx(m_Route.GetLocalPort(),
                                       &m_AmsAddr,
                                       m_IndexGroup,
                                       m_Handle,
                                       size,
                                       data);

        if (error) {
            throw AdsException(error);
        }
    }

    const AdsRoute GetRoute() const
    {
        return m_Route;
    }

    const long GetHandle() const
    {
        return m_Handle;
    }

    const uint32_t GetIndexGroup() const
    {
        return m_IndexGroup;
    }

private:
    const AdsRoute m_Route;
    const AmsAddr m_AmsAddr;
    const uint32_t m_IndexGroup;
    const AdsHandle m_Handle;
};
