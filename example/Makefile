OS_NAME ?=$(shell uname)
LIB_NAME = ../AdsLib-$(OS_NAME).a
CXXFLAGS += -std=c++11
CXXFLAGS += -pedantic
CXXFLAGS += -Wall
CXXFLAGS += -Wextra
CXXFLAGS += -I../AdsLib/
CXXFLAGS += $(ci_cxx_flags)

LDFLAGS += -lpthread
LDFLAGS_Darwin += -lc++
LDFLAGS_win32 += -lws2_32
LDFLAGS += $(LDFLAGS_$(OS_NAME))

all: example.bin

example.bin: example.cpp $(LIB_NAME)
	$(CXX) $^ $(LDFLAGS) $(CXXFLAGS) -o $@
	
test: example.bin
	./$<

clean:
	rm -f *.o *.bin
