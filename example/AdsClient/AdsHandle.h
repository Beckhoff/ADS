#pragma once

#include "AdsException.h"
#include "..\AdsLib.h"
#include <functional>
#include <memory>

struct HandleDeleter {
    HandleDeleter(const AmsAddr __address = AmsAddr {}, long __port = 0) : address(__address),
                                                                           port(__port)
    {}

    void operator()(uint32_t* handle)
    {
        uint32_t error = 0;
        if (port) {
            error = AdsSyncWriteReqEx(
                port,
                &address,
                ADSIGRP_SYM_RELEASEHND, 0,
                sizeof(*handle), handle
                );
        }
        delete handle;

        if (error) {
            throw AdsException(error);
        }
    }

private:
    const AmsAddr address;
    const long port;
};

class AdsHandle {
    using AdsHandleGuard = std::unique_ptr<uint32_t, HandleDeleter>;

    static AdsHandleGuard GetHandle(const AmsAddr address, long port,
                                    const std::string& symbolName)
    {
        uint32_t handle = 0;
        uint32_t bytesRead = 0;
        uint32_t error = AdsSyncReadWriteReqEx2(
            port,
            &address,
            ADSIGRP_SYM_HNDBYNAME, 0,
            sizeof(handle), &handle,
            symbolName.size(),
            symbolName.c_str(),
            &bytesRead
            );

        if (error || (sizeof(handle) != bytesRead)) {
            throw AdsException(error);
        }

        return AdsHandleGuard {new uint32_t {handle}, HandleDeleter {address, port}};
    }

    AdsHandleGuard m_Handle;
public:
    AdsHandle(uint32_t offset)
        : m_Handle{new uint32_t {offset}, HandleDeleter {}}
    {}

    AdsHandle(const AmsAddr address, long port,
              const std::string& symbolName)
        : m_Handle{GetHandle(address, port, symbolName)}
    {}

    operator uint32_t() const
    {
        return *m_Handle;
    }
};
