#pragma once
#include <cstdint>
#include <memory>

struct AdsRoute;

struct HandleDeleter {
    virtual void operator()(uint32_t* handle);
};

struct SymbolHandleDeleter : public HandleDeleter {
    SymbolHandleDeleter(const AdsRoute& route);
    virtual void operator()(uint32_t* handle);
protected:
    const AdsRoute& m_Route;
};

struct NotificationHandleDeleter : public SymbolHandleDeleter {
    NotificationHandleDeleter(const AdsRoute& route);
    virtual void operator()(uint32_t* handle);
};

using AdsHandle = std::unique_ptr<uint32_t, HandleDeleter>;
