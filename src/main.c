/* Game loop: menu -> race -> finish screen. Physics at a fixed 30 ticks/s from clock() (32768 Hz), render once per frame. */
#include <graphx.h>
#include <keypadc.h>
#include <fileioc.h>
#include <time.h>
#include "render.h"
#include "save.h"

#define CLK 32768L             /* CE clock() rate (CLOCKS_PER_SEC); host shim fakes the same rate */
#define MAX_CATCHUP 4

enum { K_DOWN = 1, K_LEFT = 2, K_RIGHT = 4, K_UP = 8, K_2ND = 16, K_ALPHA = 32, K_MODE = 64,
       K_CLEAR = 128, K_ENTER = 256, K_GRAPH = 512 };

/* static: record_t is ~4.5 KB and the CE stack ~4 KB. race is extern so the host sim can script against it. */
static track_t trk;
static record_t best;
race_t race;
static uint16_t keys, pressed, menu_best[TRACK_COUNT];
static uint8_t show_fps, show_ghost = 1;

static void scan(void)          /* once per frame */
{
    uint16_t k;
    kb_Scan();
    k = (uint16_t)(kb_Data[7] & 15);
    if (kb_Data[1] & kb_2nd) k |= K_2ND;
    if (kb_Data[1] & kb_Mode) k |= K_MODE;
    if (kb_Data[1] & kb_Graph) k |= K_GRAPH;
    if (kb_Data[2] & kb_Alpha) k |= K_ALPHA;
    if (kb_Data[6] & kb_Enter) k |= K_ENTER;
    if (kb_Data[6] & kb_Clear) k |= K_CLEAR;
    pressed = (uint16_t)(k & ~keys);
    keys = k;
}

static int8_t menu(uint8_t sel)  /* chosen track, or -1 to quit */
{
    uint8_t dirty = 1;
    for (uint8_t i = 0; i < N_TRACKS; i++) { save_load(i, TRACKS[i].n_cp, &best); menu_best[i] = best.ticks; }
    for (;;) {
        scan();
        if (pressed & K_UP) { sel = sel ? sel - 1 : N_TRACKS - 1; dirty = 1; }
        if (pressed & K_DOWN) { sel = sel + 1 < N_TRACKS ? sel + 1 : 0; dirty = 1; }
        if (pressed & (K_2ND | K_ENTER)) return (int8_t)sel;
        if (pressed & K_CLEAR) return -1;
        if (pressed & (K_LEFT | K_RIGHT)) { show_ghost ^= 1; dirty = 1; }
        if (dirty) { render_menu(sel, menu_best, show_ghost); dirty = 0; }
    }
}

/* Shows the result; stores a new best. Returns 1 = restart, 0 = menu. */
static uint8_t finish(uint8_t idx)
{
    uint8_t nb = (uint8_t)race_is_new_best(&race), ok = 1;
    clock_t t0 = clock();
    render_finish(&race, nb);
    do scan(); while (!pressed && (uint32_t)(clock() - t0) < 2 * CLK);
    if (nb) {
        best = race.run;
        ok = (uint8_t)save_store(idx, &best);   /* failure keeps the old file; this session still races the new ghost */
        render_init();            /* archiving may GC: the GC hooks drop to the OS screen and back, so restore palette/buffer */
        render_finish(&race, ok ? 1 : 2);
    }
    while (!(pressed & (K_2ND | K_MODE | K_ENTER | K_CLEAR))) scan();
    return !(pressed & K_CLEAR);
}

static void play(uint8_t idx)
{
    track_build(&trk, &TRACKS[idx]);
    save_load(idx, trk.n_cp, &best);
    do {
        clock_t last = clock(), sec = last;
        uint32_t acc = 0;
        uint8_t frames = 0, fps = 0;
        race_start(&race, &trk, &best);
        while (!race.finished && !race.capped) {
            clock_t now = clock();
            uint8_t in = 0, n, wall = 0;
            acc += (uint32_t)(now - last) * 30;   /* accumulate in 30ths of a clock tick: tick when acc >= CLK */
            last = now;
            scan();
            if (pressed & K_CLEAR) return;
            if (pressed & K_MODE) { race_start(&race, &trk, &best); acc = 0; }   /* instant restart, same best */
            if (pressed & K_GRAPH) show_fps ^= 1;
            if (keys & (K_2ND | K_UP)) in |= IN_GAS;
            if (keys & (K_ALPHA | K_DOWN)) in |= IN_BRAKE;
            if (keys & K_LEFT) in |= IN_LEFT;
            if (keys & K_RIGHT) in |= IN_RIGHT;
            for (n = 0; acc >= (uint32_t)CLK && n < MAX_CATCHUP; n++) {
                acc -= CLK;
                race_tick(&race, in);
                wall |= race.car.wall_hit;
            }
            if (acc >= (uint32_t)CLK) acc %= CLK;   /* catch-up cap: drop the rest */
            race.car.wall_hit = wall;                /* latch: a hit on any of this frame's ticks flashes */
            frames++;
            if ((uint32_t)(now - sec) >= (uint32_t)CLK) { fps = frames; frames = 0; sec = now; }
            render_race(&race, show_fps, fps, show_ghost);
        }
    } while (finish(idx));
}

int main(void)
{
    int8_t sel = 0;
    gfx_Begin();
    render_init();
    ti_SetGCBehavior(gfx_End, gfx_Begin);   /* let the OS draw its GC screen in its own mode; finish() restores ours */
    scan();                                 /* the enter that launched the program is still held: don't count it as a menu press */
    while ((sel = menu((uint8_t)sel)) >= 0) play((uint8_t)sel);
    gfx_End();
    return 0;
}
