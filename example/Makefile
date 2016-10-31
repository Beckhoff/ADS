
LIB_DIR = ADS
LIB_NAME = $(LIB_DIR)/AdsLib/AdsLib-$(shell uname).a
OOI_LIB_NAME = $(LIB_DIR)/AdsLibOOI/AdsLibOOI-$(shell uname).a


all: example.bin

example.bin: example.cpp $(LIB_NAME) $(OOI_LIB_NAME)
	$(CXX) $^ -I$(LIB_DIR) -std=c++11 -Wall -pedantic -lpthread -o $@
	
test: example.bin
	./$<

clean:
	rm -f *.o *.bin
