#pragma once

#include "AdsRoute.h"
#include "AdsLib/AdsLib.h"
#include <functional>
#include <memory>

class AdsHandle {
    static AdsRoute::AdsHandleGuard GetHandle(const AdsRoute&    route,
                                              const std::string& symbolName)
    {
        uint32_t handle = 0;
        uint32_t bytesRead = 0;
        uint32_t error = route.ReadWriteReqEx2(
            ADSIGRP_SYM_HNDBYNAME, 0,
            sizeof(handle), &handle,
            symbolName.size(),
            symbolName.c_str(),
            &bytesRead
            );

        if (error || (sizeof(handle) != bytesRead)) {
            throw AdsException(error);
        }

        return AdsRoute::AdsHandleGuard {new uint32_t {handle}, AdsRoute::SymbolHandleDeleter {route}};
    }

    AdsRoute::AdsHandleGuard m_Handle;
public:
    AdsHandle(uint32_t offset)
        : m_Handle{new uint32_t {offset}, AdsRoute::HandleDeleter {}}
    {}

    AdsHandle(const AdsRoute& route, const std::string& symbolName)
        : m_Handle{GetHandle(route, symbolName)}
    {}

    AdsHandle(AdsHandle&& ref)
        : m_Handle{std::move(ref.m_Handle)}
    {}

    operator uint32_t() const
    {
        return *m_Handle;
    }
};
