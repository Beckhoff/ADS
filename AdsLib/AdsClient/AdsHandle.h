#pragma once

#include "AdsException.h"
#include "..\AdsLib.h"

class AdsHandle {
    static void ReleaseHandleDummy(const AmsAddr address, long port, uint32_t* handle) {}
    static void ReleaseHandle(const AmsAddr address, long port, uint32_t* handle)
    {
        uint32_t error = AdsSyncWriteReqEx(
            port,
            &address,
            ADSIGRP_SYM_RELEASEHND, 0,
            sizeof(*handle), handle
            );

        if (error) {
            throw AdsException(error);
        }
    }

    using AdsHandleGuard =
              std::unique_ptr<uint32_t,
                              decltype(std::bind(& ReleaseHandle, std::declval<const AmsAddr>(),
                                                 std::declval<long>(), std::placeholders::_1))>;

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

        return AdsHandleGuard {new uint32_t {handle}, std::bind(&ReleaseHandle, address, port, std::placeholders::_1)};
    }

    AdsHandleGuard m_Handle;
public:
    AdsHandle(uint32_t offset)
        : m_Handle{new uint32_t {offset}, std::bind(&ReleaseHandleDummy, AmsAddr {}, 0L, std::placeholders::_1)}
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
