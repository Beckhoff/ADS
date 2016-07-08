
LIB_NAME = AdsLib-$(shell uname).a

all: example.bin

example.bin: $(LIB_NAME) example.cpp
	$(CXX) example.cpp $< -std=c++11 -Wall -pedantic -lpthread -o $@
	
test: example.bin
	./$<

clean:
	rm -f *.o *.bin
