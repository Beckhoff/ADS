
OS_NAME ?=$(shell uname)
VPATH = AdsLib
LIBS = -lpthread
LIB_NAME = AdsLib-$(OS_NAME).a
OBJ_DIR = obj
INSTALL_DIR=example
CXX :=$(CROSS_COMPILE)$(CXX)
CXXFLAGS += -std=c++11
CXXFLAGS += -pedantic
CXXFLAGS += -Wall
CPPFLAGS += -I AdsLib/
CPPFLAGS += -I ../

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

ifeq ($(OS_NAME),Darwin)
	LIBS += -lc++
endif

ifeq ($(OS_NAME),win32)
	LIBS += -lws2_32
endif

all: $(LIB_NAME)

$(OBJ_DIR):
	mkdir -p $@

$(OBJ_FILES): | $(OBJ_DIR)
$(OBJ_FILES): $(OBJ_DIR)/%.o: %.cpp
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(LIB_NAME): $(OBJ_FILES)
	$(AR) rvs $@ $?

AdsLibTest.bin: AdsLibTest/main.cpp $(LIB_NAME)
	$(CXX) $^ $(LIBS) $(CPPFLAGS) $(CXXFLAGS) -o $@

test: AdsLibTest.bin
	./$<

install: $(LIB_NAME) AdsLib.h AdsDef.h
	cp $? $(INSTALL_DIR)/

clean:
	rm -f *.a *.o *.bin AdsLibTest/*.o $(OBJ_DIR)/*.o

uncrustify:
	uncrustify --no-backup -c tools/uncrustify.cfg AdsLib*/*.h AdsLib*/*.cpp example/*.cpp

prepare-hooks:
	rm -f .git/hooks/pre-commit
	ln -Fv tools/pre-commit.uncrustify .git/hooks/pre-commit
	chmod a+x .git/hooks/pre-commit

.PHONY: clean uncrustify prepare-hooks
