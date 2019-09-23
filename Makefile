.POSIX:
CFLAGS = --std=c99 -O3 -Wall -Wextra
LDFLAGS = -lm
PREFIX = /usr/local
main: main.c
	$(CC) $(CFLAGS) -Iinclude/ main.c -o aspect $(LDFLAGS)

all: main

clean: 
	rm -f aspect

install:
	cp aspect $(DESTDIR)$(PREFIX)/bin/

uninstall:
	rm $(DESTDIR)$(PREFIX)/bin/aspect

PHONY = all clean install uninstall
