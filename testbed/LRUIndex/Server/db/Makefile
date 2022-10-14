CC=g++
CFLAGS=-std=c++11 -c -O3 -Wall -fgnu-tm
LIBRARYS=-Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -mssse3 -lmlx4 -libverbs -lpthread -ldl -lm -lnuma
INCLUDES=-I../ -I/usr/local/include/dpdk
SOURCES=$(wildcard *.cc)
OBJECTS=$(SOURCES:.cc=.o)

all: $(SOURCES) $(OBJECTS)

.cc.o:
	$(CC) $(CFLAGS) $(INCLUDES) $(LIBRARYS) $< -o $@

clean:
	$(RM) $(OBJECTS)

