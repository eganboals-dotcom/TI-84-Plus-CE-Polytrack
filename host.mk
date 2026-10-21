# Host build: logic tests (`test`) and the screenshot driver (`sim`).
CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Werror -O1 -Isrc
LOGIC = src/track.c src/physics.c src/race.c src/savefmt.c
PLAT = src/save.c src/render.c host/shim/shim.c
TESTS = $(wildcard tests/*.c)

.PHONY: test sim
test: build/tests
	./build/tests

build/tests: $(TESTS) $(LOGIC) $(PLAT) $(wildcard src/*.h tests/*.h host/shim/*.h)
	@mkdir -p build
	$(CC) $(CFLAGS) -Ihost/shim -o $@ $(TESTS) $(LOGIC) $(PLAT)

sim: build/sim
	@mkdir -p build/shots

build/sim: host/sim.c $(LOGIC) $(PLAT) $(wildcard src/*.h host/shim/*.h)
	@mkdir -p build
	$(CC) $(CFLAGS) -Ihost/shim -o $@ host/sim.c $(LOGIC) $(PLAT)
