#include "AdsRoute.h"
#include "AdsException.h"
#include "AdsLib/AdsLib.h"

void HandleDeleter::operator()(uint32_t* handle)
{
    delete handle;
}

SymbolHandleDeleter::SymbolHandleDeleter(const AdsRoute& route)
    : m_Route(route)
{}

void SymbolHandleDeleter::operator()(uint32_t* handle)
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

NotificationHandleDeleter::NotificationHandleDeleter(const AdsRoute& route)
    : SymbolHandleDeleter(route)
{}

void NotificationHandleDeleter::operator()(uint32_t* handle)
{
    uint32_t error = 0;
    if (handle && *handle) {
        error = AdsSyncDelDeviceNotificationReqEx(m_Route.GetLocalPort(), &m_Route.m_Addr, *handle);
    }
    delete handle;

    if (error) {
        throw AdsException(error);
    }
}
