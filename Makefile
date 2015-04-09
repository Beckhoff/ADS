
VPATH = AdsLib
LIBS = -lpthread
CC = g++
LIB_NAME = AdsLib-$(shell uname).a

ifeq ($(shell uname),Darwin)
	CC = clang
	LIBS += -lc++
endif

all: AdsLibTest.bin

.cpp.o:
	$(CC) -Wall -pedantic -c -g -std=c++11 $< -o $@ -I AdsLib/

$(LIB_NAME): AdsLib.o AmsConnection.o AmsPort.o AmsRouter.o Log.o Sockets.o Frame.o
	ar rvs $@ $?

AdsLibTest.bin: $(LIB_NAME)
	$(CC) AdsLibTest/main.cpp $< -I AdsLib/ -I ../ -std=c++11 $(LIBS) -o $@
	
test: AdsLibTest.bin
	./$<

release: $(LIB_NAME) AdsLib.h AdsDef.h
	cp $? example/

clean:
	rm -f *.a *.o *.bin
