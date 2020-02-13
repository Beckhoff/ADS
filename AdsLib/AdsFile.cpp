// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2020 Beckhoff Automation GmbH & Co. KG
 */

#include "AdsFile.h"

AdsFile::AdsFile(const AdsDevice& route, const std::string& filename, const uint32_t flags)
    : m_Route(route),
    m_Handle(route.OpenFile(filename, flags))
{}

void AdsFile::Delete(const AdsDevice& route, const std::string& filename, const uint32_t flags)
{
    auto error = route.ReadWriteReqEx2(SYSTEMSERVICE_FDELETE,
                                       flags,
                                       0,
                                       nullptr,
                                       filename.length(),
                                       filename.c_str(),
                                       nullptr);

    if (error) {
        throw AdsException(error);
    }
}

void AdsFile::Read(const size_t size, void* data, uint32_t& bytesRead) const
{
    auto error = m_Route.ReadWriteReqEx2(SYSTEMSERVICE_FREAD,
                                         *m_Handle,
                                         size,
                                         data,
                                         0,
                                         nullptr,
                                         &bytesRead);

    if (error) {
        throw AdsException(error);
    }
}

void AdsFile::Write(const size_t size, const void* data) const
{
    auto error = m_Route.ReadWriteReqEx2(SYSTEMSERVICE_FWRITE, *m_Handle, 0, nullptr, size, data, nullptr);
    if (error) {
        throw AdsException(error);
    }
}
