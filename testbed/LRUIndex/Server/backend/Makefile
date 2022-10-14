target = backend ../driver/driver.o ../driver/flow_blocks.o

CC = g++
LD_LIBRARY = -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -mssse3 -lmlx4 -libverbs -lpthread -ldl -lm -lnuma
LD_INCLUDE = -I/usr/local/include/dpdk/
CFLAGS = -O3 #-DDEBUG

all: backend

backend: backend.cc ../driver/driver.o ../driver/flow_blocks.o
	$(CC) $(CFLAGS) $(LD_LIBRARY) $(LD_INCLUDE) $^ -o $@

../driver/driver.o: ../driver/driver.cc
	$(CC) $(CFLAGS) $(LD_LIBRARY) $(LD_INCLUDE) $^ -c -o $@

../driver/flow_blocks.o: ../driver/flow_blocks.cc
	$(CC) $(CFLAGS) $(LD_LIBRARY) $(LD_INCLUDE) $^ -c -o $@

.PHONY: clean
clean:
	-rm $(target)
