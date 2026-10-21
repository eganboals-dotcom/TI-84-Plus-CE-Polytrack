/* Host driver: runs the real game loop (src/main.c built as game_main) with a scripted keypad,
   advancing the fake clock one 30 fps frame per kb_Scan, and dumps PPM screenshots at key moments.
   Script: menu -> track 3 -> gas 4 s with steering -> restart -> autopilot to finish (new best)
   -> 2nd run against the ghost (late start, fps on) -> finish with delta -> menu -> quit. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <graphx.h>
#include <keypadc.h>
#include "race.h"
#include "autopilot.h"

int game_main(void);
extern race_t race;

static int phase, f, total, split_seen;

static void shot(const char *name)
{
    char p[64];
    snprintf(p, sizeof p, "build/shots/%s.ppm", name);
    if (!shim_dump_ppm(p)) fprintf(stderr, "cannot write %s\n", p);
}

static void drive(uint8_t in)
{
    if (in & IN_GAS) kb_Data[1] |= kb_2nd;
    if (in & IN_BRAKE) kb_Data[2] |= kb_Alpha;
    if (in & IN_LEFT) kb_Data[7] |= kb_Left;
    if (in & IN_RIGHT) kb_Data[7] |= kb_Right;
}

static void next(void) { phase++; f = -1; }

static void frame(void)   /* called from kb_Scan: the previous frame is on screen */
{
    shim_ticks += 1093;     /* ~32768 / 30 */
    memset(kb_Data, 0, sizeof kb_Data);
    if (++total > 20000) { fprintf(stderr, "sim: script stuck in phase %d\n", phase); exit(1); }
    f++;
    switch (phase) {
    case 0:                                         /* menu: two downs, enter */
        if (f == 2) shot("01_menu");
        if (f == 3 || f == 5) kb_Data[7] = kb_Down;
        if (f == 7) { kb_Data[6] = kb_Enter; next(); }
        break;
    case 1:                                         /* 4 s of gas with some steering, then restart */
        drive(IN_GAS | (f > 30 && f < 60 ? IN_RIGHT : f > 70 && f < 100 ? IN_LEFT : 0));
        if (f == 119) shot("02_first_run");
        if (f == 120) { kb_Data[1] = kb_Mode; next(); }
        break;
    case 2:                                         /* autopilot to the finish: first record */
        if (!race.finished) { drive(autopilot(&race.car, race.track)); f = 0; break; }
        if (f == 1) shot("03_finish_new_best");
        if (f == 76) { kb_Data[1] = kb_2nd; next(); }   /* past the 2 s wait + save */
        break;
    case 3:                                         /* 2nd run: late start so the ghost leads */
        if (f == 1) kb_Data[1] = kb_Graph;
        if (race.finished) { shot("06_finish_delta"); kb_Data[6] = kb_Clear; next(); break; }
        if (f > 20) drive(autopilot(&race.car, race.track));
        if (f == 60) shot("04_ghost");
        if (race.last_split_cp != 0xFF && !split_seen && ++split_seen) f = 1000;
        if (f == 1006) shot("05_split");
        break;
    case 4:                                         /* menu with the saved best, then quit */
        if (f == 2) shot("07_menu_best");
        if (f == 4) kb_Data[6] = kb_Clear;
        break;
    }
}

int main(void)
{
    shim_kb_hook = frame;
    game_main();
    printf("sim: %d frames, shots in build/shots\n", total);
    return phase == 4 ? 0 : 1;
}
