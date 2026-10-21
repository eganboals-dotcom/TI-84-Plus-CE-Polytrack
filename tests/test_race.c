#include <string.h>
#include "test.h"
#include "race.h"

static track_t trk;
static race_t R, R2;
static record_t NOBEST, BEST;

static void setup(void)
{
    trk.n_segs = 300; trk.n_cp = 2; trk.cp_seg[0] = 100; trk.cp_seg[1] = 200;
    for (int i = 0; i < 300; i++) { trk.curve[i] = 0; trk.y[i] = 0; }
    memset(&NOBEST, 0, sizeof NOBEST);
}
static uint8_t seq(int i) { return (uint8_t)(IN_GAS | ((i / 40) % 3 == 1 ? IN_LEFT : (i / 40) % 3 == 2 ? IN_RIGHT : 0)); }

void test_pack_roundtrip(void)
{
    setup(); race_start(&R, &trk, &NOBEST);
    uint32_t r = 7;
    for (int i = 0; i < 1000; i++) { r = r * 1103515245u + 12345u; race_tick(&R, (r >> 16) & 15); }
    r = 7;
    for (int i = 0; i < 1000; i++) { r = r * 1103515245u + 12345u; CHECK(record_input(&R.run, i) == ((r >> 16) & 15)); }
}

void test_ghost_replays_exactly(void)
{
    setup(); race_start(&R, &trk, &NOBEST);
    for (int i = 0; i < MAX_TICKS && !R.finished; i++) race_tick(&R, seq(i));
    CHECK(R.finished);
    BEST = R.run;
    race_start(&R2, &trk, &BEST);
    for (int i = 0; i < BEST.ticks; i++) {
        race_tick(&R2, seq(i));
        CHECK(R2.ghost.z == R2.car.z && R2.ghost.zfrac == R2.car.zfrac && R2.ghost.x == R2.car.x && R2.ghost.speed == R2.car.speed);
    }
    CHECK(R2.finished && R2.run.ticks == BEST.ticks && !race_is_new_best(&R2));
}

void test_checkpoint_once(void)
{
    setup(); race_start(&R, &trk, &NOBEST);
    while (R.next_cp == 0 && R.tick < 3000) race_tick(&R, IN_GAS);
    CHECK(R.next_cp == 1);
    uint16_t s0 = R.run.splits[0];
    CHECK(s0 == R.tick && R.last_split_cp == 0);
    R.car.z = 100 * SEG_LEN - 500;                   /* back before cp0 */
    race_tick(&R, IN_GAS);
    CHECK(R.next_cp == 1 && R.run.splits[0] == s0);
    while (R.car.z < 100 * SEG_LEN + 100) race_tick(&R, IN_GAS);
    CHECK(R.next_cp == 1 && R.run.splits[0] == s0);
}

void test_finish_sets_ticks(void)
{
    setup(); race_start(&R, &trk, &NOBEST);
    CHECK(R.last_split_cp == 0xFF && R.run.n_cp == 2);
    while (!R.finished && R.tick < MAX_TICKS) race_tick(&R, IN_GAS);
    CHECK(R.finished && !R.capped && R.run.ticks == R.tick && race_is_new_best(&R));
    race_t snap = R;
    race_tick(&R, IN_GAS); race_tick(&R, 0);
    CHECK(memcmp(&snap, &R, sizeof R) == 0);
}

void test_cap_run(void)
{
    setup(); race_start(&R, &trk, &NOBEST);
    for (int i = 0; i < MAX_TICKS + 10; i++) race_tick(&R, 0);
    CHECK(R.capped == 1 && R.tick == MAX_TICKS && !R.finished && !race_is_new_best(&R));
}

void test_format(void)
{
    char b[10];
    format_time(b, 0);    CHECK(strcmp(b, "0:00.00") == 0);
    format_time(b, 30);   CHECK(strcmp(b, "0:01.00") == 0);
    format_time(b, 1835); CHECK(strcmp(b, "1:01.16") == 0);
    format_delta(b, 14);  CHECK(strcmp(b, "+0.46") == 0);
    format_delta(b, -36); CHECK(strcmp(b, "-1.20") == 0);
    format_delta(b, 370); CHECK(strcmp(b, "+12.33") == 0);
    format_delta(b, -9000); CHECK(strcmp(b, "-300.00") == 0);
}

void test_split_delta(void)
{
    setup(); memset(&BEST, 0, sizeof BEST); BEST.ticks = 500; BEST.splits[0] = 100;
    race_start(&R, &trk, &BEST);
    R.tick = 109; R.car.z = 100 * SEG_LEN;
    race_tick(&R, 0);
    CHECK(R.last_split_delta == 10 && R.last_split_cp == 0 && R.run.splits[0] == 110);
}
