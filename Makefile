OS_NAME ?= $(shell uname)
VPATH = AdsLib
LIB_NAME = AdsLib-$(OS_NAME).a
OBJ_DIR = obj
CXX :=$(CROSS_COMPILE)$(CXX)
CXXFLAGS += -std=c++11
CXXFLAGS += -pedantic
CXXFLAGS += -Wall
CXXFLAGS += -Wextra
CXXFLAGS += -D_GNU_SOURCE
CXXFLAGS += $(ci_cxx_flags)
CPPFLAGS += -I AdsLib/
CPPFLAGS += -I tools/

SRC_FILES += AdsDef.cpp
SRC_FILES += AdsDevice.cpp
SRC_FILES += Log.cpp
SRC_FILES += Sockets.cpp
SRC_FILES += Frame.cpp
OBJ_FILES = $(SRC_FILES:%.cpp=$(OBJ_DIR)/%.o)

# simple router implementation required for systems without TwinCAT
ROUTER_FILES += standalone/AdsLib.cpp
ROUTER_FILES += standalone/AmsConnection.cpp
ROUTER_FILES += standalone/AmsNetId.cpp
ROUTER_FILES += standalone/AmsPort.cpp
ROUTER_FILES += standalone/AmsRouter.cpp
ROUTER_FILES += standalone/NotificationDispatcher.cpp
ROUTER_OBJ = $(ROUTER_FILES:%.cpp=$(OBJ_DIR)/%.o)

LDFLAGS += -lpthread
LDFLAGS_Darwin += -lc++
LDFLAGS_win32 += -lws2_32
LDFLAGS += $(LDFLAGS_$(OS_NAME))

all: $(LIB_NAME)

$(OBJ_DIR):
	mkdir -p $@/standalone

$(OBJ_DIR)/%.o: %.cpp | $(OBJ_DIR)
	$(CXX) -c $(CPPFLAGS) $(CXXFLAGS) $< -o $@

$(LIB_NAME): $(OBJ_FILES) $(ROUTER_OBJ)
	$(AR) rvs $@ $^

AdsLibTest.bin: AdsLibTest/main.cpp $(LIB_NAME)
	$(CXX) $^ $(LDFLAGS) $(CPPFLAGS) $(CXXFLAGS) -o $@

AdsLibOOITest.bin: AdsLibOOITest/main.cpp $(LIB_NAME)
	$(CXX) $^ $(LDFLAGS) $(CPPFLAGS) $(CXXFLAGS) -o $@

test: AdsLibTest.bin
	./$<

testOOI: AdsLibOOITest.bin
	./$<

clean:
	rm -rf *.a *.o *.bin AdsLib*Test/*.o $(OBJ_DIR)

uncrustify:
	find AdsLib* example -name *.h -or -name *.cpp | uncrustify --no-backup -c tools/uncrustify.cfg -F -

prepare-hooks:
	rm -f .git/hooks/pre-commit
	ln -Fv tools/pre-commit.uncrustify .git/hooks/pre-commit
	chmod a+x .git/hooks/pre-commit

.PHONY: clean uncrustify prepare-hooks
