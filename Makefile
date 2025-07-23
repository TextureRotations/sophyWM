CFLAGS += -std=c99 -Wall -Wextra -pedantic -Wold-style-declaration
CFLAGS += -Wmissing-prototypes -Wno-unused-parameter
PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin
CC     ?= gcc

all: sophy

config.h:
	cp config.def.h config.h

sophy: sophy.c sophy.h config.h Makefile
	$(CC) -O3 $(CFLAGS) -o $@ $< -lX11 $(LDFLAGS)

install: all
	install -Dm755 sowm $(DESTDIR)$(BINDIR)/sophy

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/sophy

clean:
	rm -f sophy *.o

.PHONY: all install uninstall clean
