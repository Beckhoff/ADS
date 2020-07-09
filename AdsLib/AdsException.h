#pragma once

#include <stdexcept>
#include <string>

struct AdsException : std::exception {
    AdsException(const long adsErrorCode) :
        errorCode(adsErrorCode),
        m_Message("Ads operation failed with error code " + std::to_string(adsErrorCode) + ".")
    {}

    virtual ~AdsException() throw (){}

    virtual const char* what() const throw ()
    {
        return m_Message.c_str();
    }

    const long errorCode;
protected:
    const std::string m_Message;
};
