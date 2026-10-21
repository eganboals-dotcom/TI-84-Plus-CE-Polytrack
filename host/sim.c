/* Temporary Task 5 driver: renders a few frames per track through the graphx shim and dumps PPM screenshots. */
#include <stdio.h>
#include <graphx.h>
#include "render.h"

static track_t trk;
static record_t best;
static race_t race;

static uint8_t autopilot(const race_t *r)   /* gas + steer back toward the center line */
{
    return (uint8_t)(IN_GAS | (r->car.x > 48 ? IN_LEFT : r->car.x < -48 ? IN_RIGHT : 0));
}

static void step(uint8_t in) { race_tick(&race, in); render_race(&race, 1, 30); }

static void shot(const char *name)
{
    char p[64];
    snprintf(p, sizeof p, "build/shots/%s.ppm", name);
    if (!shim_dump_ppm(p)) { fprintf(stderr, "cannot write %s\n", p); }
}

int main(void)
{
    char name[32];
    gfx_Begin();
    render_init();
    for (uint8_t i = 0; i < N_TRACKS; i++) {
        track_build(&trk, &TRACKS[i]);
        race_start(&race, &trk, NULL);              /* best lap = autopilot from the start */
        while (!race.finished && !race.capped) race_tick(&race, autopilot(&race));
        best = race.run;
        race_start(&race, &trk, &best);
        for (int k = 0; k < 30; k++) step(0);      /* idle so the ghost pulls ahead */
        for (int k = 0; k < 90; k++) step(autopilot(&race));
        snprintf(name, sizeof name, "track%u", i);
        shot(name);
        if (i == 1) {                               /* climbing to the first crest (segment 60) */
            while (race.car.z < 60L * SEG_LEN - 2500) step(autopilot(&race));
            shot("track1_crest");
            while (race.car.z < 60L * SEG_LEN + 1500) step(autopilot(&race));
            shot("track1_over");
        }
        if (i == 0) {                               /* gate, split HUD and finish views on the warmup track */
            while (race.car.z < (int32_t)trk.cp_seg[0] * SEG_LEN - 3000) step(autopilot(&race));
            shot("track0_gate");
            while (race.last_split_cp == 0xFF) step(autopilot(&race));
            for (int k = 0; k < 10; k++) step(autopilot(&race));
            shot("track0_split");
            while (race.car.z < track_length(&trk) - 4000) step(autopilot(&race));
            shot("track0_finish");
        }
    }
    gfx_End();
    printf("shots written to build/shots\n");
    return 0;
}
