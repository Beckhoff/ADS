
OS_NAME ?=$(shell uname)
VPATH = AdsLib
LIBS = -lpthread
LIB_NAME = AdsLib-$(OS_NAME).a
INSTALL_DIR=example
CXX :=$(CROSS_COMPILE)$(CXX)
CFLAGS += -std=c++11
CFLAGS += -pedantic
CFLAGS += -Wall

ifeq ($(OS_NAME),Darwin)
	LIBS += -lc++
endif

ifeq ($(OS_NAME),win32)
	LIBS += -lws2_32
endif


.cpp.o:
	$(CXX) -c $(CFLAGS) $< -o $@ -I AdsLib/ -I ../

$(LIB_NAME): AdsDef.o AdsLib.o AmsConnection.o AmsPort.o AmsRouter.o Log.o NotificationDispatcher.o Sockets.o Frame.o
	$(AR) rvs $@ $?

AdsLibTest.bin: AdsLibTest/main.o $(LIB_NAME)
	$(CXX) $^ $(LIBS) -o $@

test: AdsLibTest.bin
	./$<

install: $(LIB_NAME) AdsLib.h AdsDef.h AdsClient
	cp --recursive $? $(INSTALL_DIR)/

clean:
	rm -f *.a *.o *.bin AdsLibTest/*.o

uncrustify:
	uncrustify --no-backup -c tools/uncrustify.cfg AdsLib*/*.h AdsLib*/*.cpp example/*.cpp AdsLib*/AdsClient/*

prepare-hooks:
	rm -f .git/hooks/pre-commit
	ln -Fv tools/pre-commit.uncrustify .git/hooks/pre-commit
	chmod a+x .git/hooks/pre-commit

.PHONY: clean uncrustify prepare-hooks
