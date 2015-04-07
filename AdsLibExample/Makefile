
CC = g++

all: example.bin

example.bin: AdsLib.a
	$(CC) example.cpp $< -std=c++11 -lpthread -o $@
	
test: example.bin
	./$<

clean:
	rm -f *.o *.bin
