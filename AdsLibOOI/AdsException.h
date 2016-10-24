#pragma once

#include <stdexcept>
#include <string>

struct AdsException : std::exception {
    AdsException(const long adsErrorCode) :
        m_AdsErrorCode(adsErrorCode),
        m_Message("Ads operation failed with error code " + std::to_string(adsErrorCode) + ".")
    {}

    virtual ~AdsException() throw (){}

    virtual const char* what() const throw ()
    {
        return m_Message.c_str();
    }

    const long getErrorCode() const
    {
        return m_AdsErrorCode;
    }

protected:
    const long m_AdsErrorCode;
    const std::string m_Message;
};
