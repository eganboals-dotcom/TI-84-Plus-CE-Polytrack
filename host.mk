# Host build for logic tests. `sim` target arrives in Task 5.
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Werror -O1 -Isrc
LOGIC = src/track.c
TESTS = $(wildcard tests/*.c)

.PHONY: test
test: build/tests
	./build/tests

build/tests: $(TESTS) $(LOGIC) $(wildcard src/*.h tests/*.h)
	@mkdir -p build
	$(CC) $(CFLAGS) -o $@ $(TESTS) $(LOGIC)
