target = sender recver ../driver/driver.o

CC = g++
LD_LIBRARY = -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -mssse3 -lmlx4 -libverbs -lpthread -ldl -lm -lnuma
LD_INCLUDE = -I/usr/local/include/dpdk/
CFLAGS = -O3 #-DDEBUG

all: sender recver

../driver/driver.o: ../driver/driver.cc
	$(CC) $(CFLAGS) $(LD_LIBRARY) $(LD_INCLUDE) $^ -c -o $@

sender: sender.cc ../driver/driver.o
	$(CC) $(CFLAGS) $(LD_LIBRARY) $(LD_INCLUDE) $^ -o $@

recver: recver.cc ../driver/driver.o
	$(CC) $(CFLAGS) $(LD_LIBRARY) $(LD_INCLUDE) $^ -o $@


.PHONY: clean
clean:
	-rm $(target)
