OS_NAME ?=$(shell uname)
PROGRAM = example.bin
$(PROGRAM): LIB_NAME = ../AdsLib-$(OS_NAME).a
Tc$(PROGRAM): LIB_NAME = ../TcAdsLib-$(OS_NAME).a
CXXFLAGS += -std=c++11
CXXFLAGS += -pedantic
CXXFLAGS += -Wall
CXXFLAGS += -Wextra
CXXFLAGS += -I../AdsLib/
Tc$(PROGRAM): CPPFLAGS += -DUSE_TWINCAT_ROUTER
Tc$(PROGRAM): CPPFLAGS += -I/usr/local/include
CXXFLAGS += $(ci_cxx_flags)

LDFLAGS += -lpthread
LDFLAGS_Darwin += -lc++
LDFLAGS_win32 += -lws2_32
LDFLAGS += $(LDFLAGS_$(OS_NAME))

all: $(PROGRAM)

$(PROGRAM): example.cpp ../AdsLib-$(OS_NAME).a
	$(CXX) $^ -v $(LDFLAGS) $(CPPFLAGS) $(CXXFLAGS) -o $@

Tc$(PROGRAM): /usr/local/lib/libTcAdsDll.so ../TcAdsLib-$(OS_NAME).a example.cpp
	$(CXX) $^ $(LDFLAGS) $(CPPFLAGS) $(CXXFLAGS) -o $@
	
test: example.bin
	./$<

clean:
	rm -f *.o *.bin
