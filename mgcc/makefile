# Makefile for mgcc wrapper

CC = gcc
CFLAGS = -O2 -Wall
TARGET = mgcc
SRC = mgcc.c
PREFIX = /usr/local

.PHONY: all install clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

install: $(TARGET)
	@echo "Installing $(TARGET) to $(PREFIX)/bin"
	cp $(TARGET) $(PREFIX)/bin/
	chmod +x $(PREFIX)/bin/$(TARGET)

clean:
	rm -f $(TARGET)