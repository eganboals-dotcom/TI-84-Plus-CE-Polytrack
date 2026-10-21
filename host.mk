# Host build for logic tests. `sim` target arrives in Task 5.
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Werror -O1 -Isrc
LOGIC = src/track.c src/physics.c src/race.c src/savefmt.c
PLAT = src/save.c host/shim/shim.c
TESTS = $(wildcard tests/*.c)

.PHONY: test
test: build/tests
	./build/tests

build/tests: $(TESTS) $(LOGIC) $(PLAT) $(wildcard src/*.h tests/*.h host/shim/*.h)
	@mkdir -p build
	$(CC) $(CFLAGS) -Ihost/shim -o $@ $(TESTS) $(LOGIC) $(PLAT)
