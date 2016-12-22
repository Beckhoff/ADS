#pragma once
#include "AdsException.h"
#include "AdsLib/AdsDef.h"
#include <memory>

struct AdsRoute {
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

    AdsRoute(const std::string& ipV4, AmsNetId netId, uint16_t port);

    long GetLocalPort() const;
    void SetTimeout(const uint32_t timeout) const;
    uint32_t GetTimeout() const;

    const std::shared_ptr<const AmsNetId> m_NetId;
    const AmsAddr m_Port;

    long ReadReqEx2(uint32_t group, uint32_t offset, uint32_t length, void* buffer, uint32_t& bytesRead) const;
    long ReadWriteReqEx2(uint32_t    indexGroup,
                         uint32_t    indexOffset,
                         uint32_t    readLength,
                         void*       readData,
                         uint32_t    writeLength,
                         const void* writeData,
                         uint32_t*   bytesRead) const;
    long WriteReqEx(uint32_t group, uint32_t offset, uint32_t length, const void* buffer) const;
private:
    std::shared_ptr<long> m_LocalPort;
};
