
VPATH = AdsLib

all: AdsLibTest.bin

.cpp.o:
	g++ -c -g -std=c++11 $< -o $@ -I AdsLib/

AdsLib.a: AdsLib.o AmsConnection.o AmsRouter.o Log.o Sockets.o Frame.o
	ar rvs $@ $?

AdsLibTest.bin: AdsLib.a
	g++ AdsLibTest/main.cpp $< -I AdsLib/ -I ../ -std=c++11 -lpthread -o $@
	
test: AdsLibTest.bin
	./$<

clean:
	rm -f *.a *.o *.bin
