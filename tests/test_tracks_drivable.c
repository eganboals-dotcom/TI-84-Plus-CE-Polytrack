#include "test.h"
#include "race.h"
#include "autopilot.h"

void test_every_track_finishable(void)
{
    static track_t t;
    static race_t r;
    uint16_t prev = 0;
    for (int i = 0; i < N_TRACKS; i++) {
        track_build(&t, &TRACKS[i]);
        race_start(&r, &t, NULL);
        while (!r.finished && !r.capped) race_tick(&r, autopilot(&r.car, &t));
        printf("  track %d %-14s %4u segs  %4u ticks (%u.%02us)\n", i, TRACKS[i].name, t.n_segs, r.tick,
               r.tick / 30, r.tick % 30 * 100 / 30);
        CHECK(r.finished && !r.capped);
        CHECK(r.tick >= 900 && r.tick <= 1800);
        if (i) CHECK(r.tick * 10 >= prev * 9);
        prev = r.tick;
    }
}
