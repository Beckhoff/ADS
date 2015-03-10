
all: AdsLib/AdsLib.a
	g++ AdsLibTest/main.cpp $< -I AdsLib/ -I ../ -std=c++11 -lpthread

AdsLib/AdsLib.a:
	cd AdsLib/
	make
	cd ..
	
