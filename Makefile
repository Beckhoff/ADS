
VPATH = AdsLib
LIBS = -lpthread
CC = g++

ifeq ($(shell uname),Darwin)
	CC = clang
	LIBS += -lc++
endif

all: AdsLibTest.bin

.cpp.o:
	$(CC) -Wall -pedantic -c -g -std=c++11 $< -o $@ -I AdsLib/

AdsLib.a: AdsLib.o AmsConnection.o AmsRouter.o Log.o Sockets.o Frame.o
	ar rvs $@ $?

AdsLibTest.bin: AdsLib.a
	$(CC) AdsLibTest/main.cpp $< -I AdsLib/ -I ../ -std=c++11 $(LIBS) -o $@
	
test: AdsLibTest.bin
	./$<

release: AdsLib.a AdsLib.h AdsDef.h
	cp $? AdsLibExample/

clean:
	rm -f *.a *.o *.bin
