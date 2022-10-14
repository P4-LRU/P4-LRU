CC=g++
CFLAGS=-std=c++11 -O3 -Wall -pthread -I./
LIBRARYS=-Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -mssse3 -lmlx4 -libverbs -lpthread -ldl -lm -lnuma
LDFLAGS= -lpthread -ltbb -lhiredis
SUBDIRS=core db redis driver
SUBSRCS=$(wildcard core/*.cc) $(wildcard db/*.cc) $(wildcard driver/*.cc)
OBJECTS=$(SUBSRCS:.cc=.o)
EXEC=ycsbc

all: $(SUBDIRS) $(EXEC)

$(SUBDIRS):
	$(MAKE) -C $@

$(EXEC): $(wildcard *.cc) $(OBJECTS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) $(LIBRARYS) -o $@

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir $@; \
	done
	$(RM) $(EXEC)

.PHONY: $(SUBDIRS) $(EXEC)
