OS ?=$(shell uname)
LIBS = -lpthread
LIB_NAME = ../AdsLib-$(OS).a
CXXFLAGS += -std=c++11
CXXFLAGS += -pedantic
CXXFLAGS += -Wall
CXXFLAGS += -I../AdsLib/
CXXFLAGS += $(ci_cxx_flags)

ifeq ($(OS),win32)
        LIBS += -lws2_32
endif

all: example.bin

example.bin: $(LIB_NAME) example.cpp
	$(CXX) example.cpp $< $(LIBS) $(CXXFLAGS) -o $@
	
test: example.bin
	./$<

clean:
	rm -f *.o *.bin
