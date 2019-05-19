help:
	@echo "targets debug release"

clean:
	rm -f main

INCDIR=include/
LIBDIR=lib/
SRCDIR=src/
SRCS=$(wildcard $(SRCDIR)*.c)

CFLAGS=-Wall -Wextra -Wfatal-errors -D_POSIX_C_SOURCE=200809L\
	-std=c11
LDFLAGS=-L$(LIBDIR)
LDLIBS=-lsighandler -lbtree -llinkedlist
IFLAGS=-I$(INCDIR)

debug:	CFLAGS += -g -Og
release:	CFLAGS += -flto -Os -s

debug:	objs
release:	objs


objs:
	gcc $(LDFLAGS) $(CFLAGS) $(IFLAGS) $(SRCS) -o main $(LDLIBS)
