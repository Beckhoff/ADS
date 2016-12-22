#pragma once

#include "AdsException.h"
#include "AdsRoute.h"
#include "AdsLib/AdsLib.h"
#include <functional>
#include <memory>

struct HandleDeleter {
    virtual void operator()(uint32_t* handle)
    {
        delete handle;
    }
};
using AdsHandleGuard = std::unique_ptr<uint32_t, HandleDeleter>;

struct SymbolHandleDeleter : public HandleDeleter {
    SymbolHandleDeleter(const AdsRoute& route)
        : m_Route(route)
    {}

    virtual void operator()(uint32_t* handle)
    {
        uint32_t error = m_Route.WriteReqEx(
            ADSIGRP_SYM_RELEASEHND, 0,
            sizeof(*handle), handle
            );
        delete handle;

        if (error) {
            throw AdsException(error);
        }
    }

protected:
    const AdsRoute& m_Route;
};

class AdsHandle {
    static AdsHandleGuard GetHandle(const AdsRoute&    route,
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

        return AdsHandleGuard {new uint32_t {handle}, SymbolHandleDeleter {route}};
    }

    AdsHandleGuard m_Handle;
public:
    AdsHandle(uint32_t offset)
        : m_Handle{new uint32_t {offset}, HandleDeleter {}}
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
