
OS_NAME ?=$(shell uname)
VPATH = AdsLib
VPATH += AdsLibOOI
LIBS = -lpthread
LIB_NAME = AdsLib-$(OS_NAME).a
OOI_LIB_NAME = AdsLibOOI-$(OS_NAME).a
INSTALL_DIR=example/ADS
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
	$(CXX) -c $(CFLAGS) $< -o $@ -I AdsLib/ -I ../ -I ./

$(LIB_NAME): AdsDef.o AdsLib.o AmsConnection.o AmsPort.o AmsRouter.o Log.o NotificationDispatcher.o Sockets.o Frame.o
	$(AR) rvs $@ $?

$(OOI_LIB_NAME): AdsDevice.o AdsNotification.o AdsRoute.o
	$(AR) rvs $@ $?

AdsLibTest.bin: AdsLibTest/main.o $(LIB_NAME)
	$(CXX) $^ $(LIBS) -o $@

AdsLibOOITest.bin: AdsLibOOITest/main.o $(OOI_LIB_NAME) $(LIB_NAME)
	$(CXX) $^ $(LIBS) -o $@

test: AdsLibTest.bin
	./$<

testOOI: AdsLibOOITest.bin
	./$<

install_lib: $(LIB_NAME) AdsLib.h AdsDef.h
	mkdir -p $(INSTALL_DIR)/AdsLib
	cp $? $(INSTALL_DIR)/AdsLib

install_ooi: $(OOI_LIB_NAME) AdsLibOOI/*.h
	mkdir -p $(INSTALL_DIR)/AdsLibOOI
	cp $? $(INSTALL_DIR)/AdsLibOOI

install: install_lib install_ooi

clean:
	rm -rf *.a *.o *.bin AdsLib*Test/*.o $(INSTALL_DIR)

uncrustify:
	uncrustify --no-backup -c tools/uncrustify.cfg AdsLib*/*.h AdsLib*/*.cpp example/*.cpp

prepare-hooks:
	rm -f .git/hooks/pre-commit
	ln -Fv tools/pre-commit.uncrustify .git/hooks/pre-commit
	chmod a+x .git/hooks/pre-commit

.PHONY: clean uncrustify prepare-hooks
