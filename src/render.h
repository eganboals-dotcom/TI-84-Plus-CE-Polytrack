#ifndef RENDER_H
#define RENDER_H
#include <stdint.h>
#include "race.h"

#define HORIZON_Y  110     /* screen row of the horizon for flat road */
#define CAM_H      1000    /* camera height above road (world units) */
#define DRAW_SEGS  70

void render_init(void);                         /* palette */
extern uint8_t half_rows;      /* experiment toggle (y= key) */
extern uint16_t prof[4];       /* frame timing, 32768 Hz ticks: swap wait, road, other draw, physics */
void render_race(const race_t *r, uint8_t show_fps, uint8_t fps, uint8_t show_ghost);
void render_finish(const race_t *r, uint8_t new_best);       /* new_best: 0 no, 1 yes, 2 yes but save failed. last race frame + result panel */
void render_menu(uint8_t sel, const uint16_t *best_ticks, uint8_t show_ghost);   /* best_ticks[N_TRACKS], 0 = no record */
int  project_y(int32_t dz, int32_t dy);         /* screen y for a point dz ahead, dy above camera's road; exposed for test */
#endif
