TOP = .
include $(TOP)/configure/CONFIG

SRC_DIRS += $(TOP)/AdsLib $(TOP)/AdsLibTest

USR_CXXFLAGS_Linux += -std=c++11 -D_GNU_SOURCE -pedantic -Wall -Wextra

LIBRARY_IOC = AdsLib

ifeq ($(OS_CLASS),WIN32)
USR_CPPFLAGS += -DDLL_EXPORT=__declspec(dllexport)
endif

AdsLib_SRCS += AdsDef.cpp
AdsLib_SRCS += AdsLib.cpp
AdsLib_SRCS += AmsConnection.cpp
AdsLib_SRCS += AmsPort.cpp
AdsLib_SRCS += AmsRouter.cpp
AdsLib_SRCS += Log.cpp
AdsLib_SRCS += NotificationDispatcher.cpp
AdsLib_SRCS += Sockets.cpp
AdsLib_SRCS += Frame.cpp

INC += AdsDef.h AdsLib.h

AdsLib_SYS_LIBS_WIN32 += ws2_32

# Requires fructose test framework
#PROD_IOC += AdsLibTest

#AdsLibTest_SRCS += main.cpp
#AdsLibTest_LIBS += AdsLib
#AdsLibTest_SYS_LIBS_WIN32 += ws2_32

include $(TOP)/configure/RULES

uninstall:
distclean: realclean
