CC=g++
CFLAGS=$(shell pkg-config --cflags libdpdk librdmacm libibverbs) -O3 -g -fpermissive
LIBRARYS=$(shell pkg-config --libs libdpdk librdmacm libibverbs)
SOURCES=$(wildcard *.cc)
OBJECTS=$(SOURCES:.cc=.o)

all: $(SOURCES) $(OBJECTS)

.cc.o:
	$(CC) $(CFLAGS) $< -c -o $@ $(LIBRARYS)

clean:
	$(RM) $(OBJECTS)
