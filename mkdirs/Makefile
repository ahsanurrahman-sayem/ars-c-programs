CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c99
PREFIX = /usr/local
BINDIR = $(PREFIX)/bin

TARGET = mkdirs
SRC = mkdirs.c

.PHONY: all clean install uninstall

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

install: $(TARGET)
	install -d $(BINDIR)
	install -m 755 $(TARGET) $(BINDIR)/$(TARGET)
	@echo "$(TARGET) installed to $(BINDIR)"

uninstall:
	rm -f $(BINDIR)/$(TARGET)
	@echo "$(TARGET) uninstalled from $(BINDIR)"