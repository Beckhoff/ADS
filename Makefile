
VPATH = AdsLib
LIBS = -lpthread
LIB_NAME = AdsLib-$(shell uname).a
INSTALL_DIR=example

ifeq ($(shell uname),Darwin)
	LIBS += -lc++
endif


.cpp.o:
	$(CXX) -Wall -pedantic -c -g -std=c++11 $< -o $@ -I AdsLib/ -I ../

$(LIB_NAME): AdsDef.o AdsLib.o AmsConnection.o AmsPort.o AmsRouter.o Log.o NotificationDispatcher.o Sockets.o Frame.o
	$(AR) rvs $@ $?

AdsLibTest.bin: AdsLibTest/main.o $(LIB_NAME)
	$(CXX) $^ $(LIBS) -o $@

test: AdsLibTest.bin
	./$<

install: $(LIB_NAME) AdsLib.h AdsDef.h
	cp $? $(INSTALL_DIR)/

clean:
	rm -f *.a *.o *.bin

uncrustify:
	uncrustify --no-backup -c tools/uncrustify.cfg AdsLib*/*.h AdsLib*/*.cpp example/*.cpp

prepare-hooks:
	rm -f .git/hooks/pre-commit
	ln -Fv tools/pre-commit.uncrustify .git/hooks/pre-commit
	chmod a+x .git/hooks/pre-commit

.PHONY: clean uncrustify prepare-hooks
