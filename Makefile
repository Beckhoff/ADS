
OS_NAME ?=$(shell uname)
VPATH = AdsLib
VPATH += AdsLibOOI
LIBS = -lpthread
LIB_NAME = AdsLib-$(OS_NAME).a
OOI_LIB_NAME = AdsLibOOI-$(OS_NAME).a
INSTALL_DIR=example/ADS
OBJ_DIR = obj
CXX :=$(CROSS_COMPILE)$(CXX)
CXXFLAGS += -std=c++11
CXXFLAGS += -pedantic
CXXFLAGS += -Wall
CXXFLAGS += $(ci_cxx_flags)
CPPFLAGS += -I AdsLib/
CPPFLAGS += -I ./
CPPFLAGS += -I tools/

SRC_FILES = AdsDef.cpp
SRC_FILES += AdsLib.cpp
SRC_FILES += AmsConnection.cpp
SRC_FILES += AmsPort.cpp
SRC_FILES += AmsRouter.cpp
SRC_FILES += Log.cpp
SRC_FILES += NotificationDispatcher.cpp
SRC_FILES += Sockets.cpp
SRC_FILES += Frame.cpp
OBJ_FILES = $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

OOI_SRC_FILES = $(SRC_FILES)
OOI_SRC_FILES += AdsNotification.cpp
OOI_SRC_FILES += AdsDevice.cpp
OOI_OBJ_FILES = $(OOI_SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

ifeq ($(OS_NAME),Darwin)
	LIBS += -lc++
endif

ifeq ($(OS_NAME),win32)
	LIBS += -lws2_32
endif

all: $(LIB_NAME)

$(OBJ_DIR):
	mkdir -p $@

$(OOI_OBJ_FILES): | $(OBJ_DIR)
$(OOI_OBJ_FILES): $(OBJ_DIR)/%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(LIB_NAME): $(OBJ_FILES)
	$(AR) rvs $@ $?

$(OOI_LIB_NAME): $(OOI_OBJ_FILES)
	$(AR) rvs $@ $?

AdsLibTest.bin: AdsLibTest/main.cpp $(LIB_NAME)
	$(CXX) $^ $(LIBS) $(CPPFLAGS) $(CXXFLAGS) -o $@

AdsLibOOITest.bin: AdsLibOOITest/main.cpp $(OOI_LIB_NAME) $(LIB_NAME)
	$(CXX) $^ $(LIBS) $(CPPFLAGS) $(CXXFLAGS) -o $@

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
	rm -rf *.a *.o *.bin AdsLib*Test/*.o $(INSTALL_DIR) $(OBJ_DIR)/*.o

uncrustify:
	uncrustify --no-backup -c tools/uncrustify.cfg AdsLib*/*.h AdsLib*/*.cpp example/*.cpp

prepare-hooks:
	rm -f .git/hooks/pre-commit
	ln -Fv tools/pre-commit.uncrustify .git/hooks/pre-commit
	chmod a+x .git/hooks/pre-commit

.PHONY: clean uncrustify prepare-hooks
