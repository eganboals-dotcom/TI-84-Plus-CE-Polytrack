#ifndef RENDER_H
#define RENDER_H
#include <stdint.h>
#include "race.h"

#define HORIZON_Y  110     /* screen row of the horizon for flat road */
#define CAM_H      1000    /* camera height above road (world units) */
#define DRAW_SEGS  70

void render_init(void);                         /* palette */
void render_race(const race_t *r, uint8_t show_fps, uint8_t fps);
int  project_y(int32_t dz, int32_t dy);         /* screen y for a point dz ahead, dy above camera's road; exposed for test */
#endif
