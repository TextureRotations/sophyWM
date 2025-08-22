# Compiler flags
CC      ?= gcc
CFLAGS  += -std=c99 -Wall -Wextra -pedantic -Wold-style-declaration
CFLAGS  += -Wmissing-prototypes -Wno-unused-parameter
LDFLAGS ?= 
LDLIBS  ?= -lX11

# Installation paths
PREFIX ?= /usr
BINDIR ?= $(PREFIX)/bin

# Source and target
SRC    = sophy.c
OBJ    = $(SRC:.c=.o)
TARGET = sophy

# Default target
all: $(TARGET)

# Build executable
$(TARGET): $(OBJ) config.h Makefile
	$(CC) $(CFLAGS) -O3 -o $@ $(OBJ) $(LDFLAGS) $(LDLIBS)

# Compile objects
%.o: %.c config.h
	$(CC) $(CFLAGS) -c -o $@ $<

# Installation
install: all
	install -Dm755 $(TARGET) $(DESTDIR)$(BINDIR)/$(TARGET)

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/$(TARGET)

# Clean
clean:
	rm -f $(TARGET) $(OBJ)

.PHONY: all install uninstall clean

