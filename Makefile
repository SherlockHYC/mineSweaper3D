CC     = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
LIBS   = -lncurses
SRC    = src/main.c src/cell.c src/board.c src/game.c src/render.c src/input.c src/settings.c

.PHONY: all debug clean

all: build/minesweeper3d

# Debug build: enables DBG() macros + AddressSanitizer + UBSan
debug:
	$(MAKE) CFLAGS="$(CFLAGS) -DDEBUG -g -fsanitize=address,undefined" all

build/minesweeper3d: $(SRC) | build
	$(CC) $(CFLAGS) -o $@ $(SRC) $(LIBS)

build:
	mkdir -p build

clean:
	rm -rf build
