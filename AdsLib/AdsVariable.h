// SPDX-License-Identifier: MIT
/**
   Copyright (c) 2020 - 2022 Beckhoff Automation GmbH & Co. KG
 */

#pragma once

#include "AdsDevice.h"
#include "cstring"


struct IAdsVariable {

  virtual void operator=(const bool& value){}
  virtual void operator=(const uint8_t& value){}
  virtual void operator=(const int8_t& value){}
  virtual void operator=(const uint16_t& value){}
  virtual void operator=(const int16_t& value){}
  virtual void operator=(const uint32_t& value){}
  virtual void operator=(const int32_t& value){}
  virtual void operator=(const uint64_t& value){}
  virtual void operator=(const int64_t& value){}
  virtual void operator=(const float& value){}
  virtual void operator=(const double& value){}
 
  virtual void ReadValue(void *res){}
}; 




template<typename T>
struct AdsVariable  : public IAdsVariable{
    AdsVariable(const AdsDevice& route, const std::string& symbolName)
        : m_Route(route),
        m_IndexGroup(ADSIGRP_SYM_VALBYHND),
        m_Handle(route.GetHandle(symbolName))
    {}

    AdsVariable(const AdsDevice& route, const uint32_t group, const uint32_t offset)
        : m_Route(route),
        m_IndexGroup(group),
        m_Handle(route.GetHandle(offset))
    {}

    operator T() const
    {
        T buffer;
        Read(sizeof(buffer), &buffer);
        return buffer;
    }

    void ReadValue(void *res) override
    {
	T buffer;
    Read(sizeof(buffer), &buffer);
    memcpy(res, &buffer, sizeof(T));
    }

    void operator=(const T& value) override
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
        auto error = m_Route.ReadReqEx2(m_IndexGroup,
                                        *m_Handle,
                                        size,
                                        data,
                                        &bytesRead);

        if (error || (size != bytesRead)) {
            throw AdsException(error);
        }
    }

    void Write(const size_t size, const void* data) const
    {
        auto error = m_Route.WriteReqEx(m_IndexGroup, *m_Handle, size, data);
        if (error) {
            throw AdsException(error);
        }
    }
private:
    const AdsDevice& m_Route;
    const uint32_t m_IndexGroup;
    const AdsHandle m_Handle;
};
