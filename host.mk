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
	@mkdir -p build/shots build/appvars
	rm -f build/appvars/*
	./build/sim

# main.c's entry becomes game_main so sim.c owns main; clock() reads the shim's fake 32768 Hz clock.
build/main.o: src/main.c $(wildcard src/*.h host/shim/*.h)
	@mkdir -p build
	$(CC) $(CFLAGS) -Ihost/shim -Dmain=game_main -Dclock=shim_clock -c -o $@ src/main.c

build/sim: host/sim.c build/main.o $(LOGIC) $(PLAT) $(wildcard src/*.h host/shim/*.h tests/autopilot.h)
	@mkdir -p build
	$(CC) $(CFLAGS) -Ihost/shim -Itests -o $@ host/sim.c build/main.o $(LOGIC) $(PLAT)
