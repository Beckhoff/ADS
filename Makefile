TOP = .
include $(TOP)/configure/CONFIG

SRC_DIRS += $(TOP)/AdsLib $(TOP)/AdsLibTest

LIBRARY_IOC = AdsLib

USR_CPPFLAGS += -DDLL_EXPORT=__declspec(dllexport)

AdsLib_SRCS += AdsDef.cpp
AdsLib_SRCS += AdsLib.cpp
AdsLib_SRCS += AmsConnection.cpp
AdsLib_SRCS += AmsPort.cpp
AdsLib_SRCS += AmsRouter.cpp
AdsLib_SRCS += Log.cpp
AdsLib_SRCS += NotificationDispatcher.cpp
AdsLib_SRCS += Sockets.cpp
AdsLib_SRCS += Frame.cpp

AdsLib_SYS_LIBS_WIN32 += ws2_32

ifneq ($(findstring Linux,$(EPICS_HOST_ARCH)),)
PROD_IOC += AdsLibTest
endif

AdsLibTest_SRCS += main.cpp
AdsLibTest_LIBS += AdsLib
AdsLibTest_SYS_LIBS_WIN32 += ws2_32

include $(TOP)/configure/RULES

uninstall:
