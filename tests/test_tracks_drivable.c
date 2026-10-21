#include "test.h"
#include "race.h"
#include "autopilot.h"

static uint16_t grass;   /* ticks spent off the road in the last run */

static uint16_t run(const track_t *t, uint8_t brakes)   /* autopilot ticks; brakes=0 clears the brake bit (gas instead) */
{
    static race_t r;
    grass = 0;
    race_start(&r, t, NULL);
    while (!r.finished && !r.capped) {
        uint8_t in = autopilot(&r.car, t);
        if (!brakes && (in & IN_BRAKE)) in = (uint8_t)((in & ~IN_BRAKE) | IN_GAS);
        race_tick(&r, in);
        grass += r.car.x > FX_ONE || r.car.x < -FX_ONE;
    }
    return r.finished ? r.tick : 0xFFFF;
}

void test_every_track_finishable(void)
{
    static track_t t;
    uint16_t prev = 0;
    for (int i = 0; i < N_TRACKS; i++) {
        uint16_t ticks, nb, nb_grass;
        track_build(&t, &TRACKS[i]);
        ticks = run(&t, 1);
        nb = run(&t, 0);
        nb_grass = grass;
        printf("  track %d %-14s %4u segs  %4u ticks (%u.%02us), no-brake %u (%u ticks on grass)\n", i, TRACKS[i].name, t.n_segs, ticks,
               ticks / 30, ticks % 30 * 100 / 30, nb, nb_grass);
        CHECK(ticks >= 900 && ticks <= 1800);
        if (i) CHECK(ticks * 10 >= prev * 9);
        if (i >= 2) CHECK(nb_grass >= TICKS_PER_SEC);   /* hairpin tracks: flat out puts you on the grass; a lift keeps you on the road */
        prev = ticks;
    }
}
