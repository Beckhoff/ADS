
.cpp.o:
	g++ -g -std=c++11 -c $< -o $@
	
all: AdsLib.o AdsConnection.o AmsRouter.o Log.o Sockets.o Frame.o
	ar rvs AdsLib.a $?
