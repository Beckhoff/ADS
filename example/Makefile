OS_NAME ?=$(shell uname)
LIBS = -lpthread
LIB_NAME = ../AdsLib-$(OS_NAME).a
OOI_LIB_NAME = ../AdsLibOOI-$(OS_NAME).a
CXXFLAGS += -std=c++11
CXXFLAGS += -pedantic
CXXFLAGS += -Wall
CXXFLAGS += -Wextra
CXXFLAGS += -I../AdsLib/
CXXFLAGS += -I../AdsLibOOI/
CXXFLAGS += $(ci_cxx_flags)

ifeq ($(OS_NAME),Darwin)
	LIBS += -lc++
endif

ifeq ($(OS_NAME),win32)
        LIBS += -lws2_32
endif

all: example.bin

example.bin: example.cpp $(LIB_NAME) $(OOI_LIB_NAME)
	$(CXX) $^ $(LIBS) $(CXXFLAGS) -o $@
	
test: example.bin
	./$<

clean:
	rm -f *.o *.bin
