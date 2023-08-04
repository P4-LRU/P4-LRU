target = sender recver ../../driver/driver.o

CC = g++
CFLAGS=$(shell pkg-config --cflags libdpdk librdmacm libibverbs) -O3 -g -fpermissive
LD_LIBRARYS=$(shell pkg-config --libs libdpdk librdmacm libibverbs) -L/
LD_INCLUDE = -I../

all: sender recver

../../driver/driver.o: ../../driver/driver.cc
	$(CC) $(CFLAGS) $(LD_INCLUDE) $^ -c -o $@  

sender: sender.cc ../../driver/driver.o
	$(CC) $(CFLAGS) $(LD_INCLUDE) $^ -o $@ $(LD_LIBRARYS) 

recver: recver.cc ../../driver/driver.o
	$(CC) $(CFLAGS) $(LD_INCLUDE) $^ -o $@ $(LD_LIBRARYS) 


.PHONY: clean
clean:
	-rm $(target)
