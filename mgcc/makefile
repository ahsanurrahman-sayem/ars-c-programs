CC = clang

# Binary names
MGCC = mgcc
MCLANG = mclang

# Source paths
MGCC_SRC = mgcc.c
MCLANG_SRC = mclang.c

# Flags
COMMON_FLAGS = -Wall
GCC_FLAGS = -O2
CLANG_FLAGS = -Oz

# Logic to determine flags and target based on CC
ifeq ($(CC),clang)
	CFLAGS = $(COMMON_FLAGS) $(CLANG_FLAGS)
	TARGET = $(MCLANG)
	SRC = $(MCLANG_SRC)
else
	CFLAGS = $(COMMON_FLAGS) $(GCC_FLAGS)
	TARGET = $(MGCC)
	SRC = $(MGCC_SRC)
endif

PREFIX = /usr/local

.PHONY: all install clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<

install: $(TARGET)
	install -D -m 755 $(TARGET) $(PREFIX)/bin/$(TARGET)

clean:
	rm -f $(MGCC) $(MCLANG)