#ifndef PHYSICS_H
#define PHYSICS_H
#include <stdint.h>
#include "fx.h"
#include "track.h"

enum { IN_GAS = 1, IN_BRAKE = 2, IN_LEFT = 4, IN_RIGHT = 8 };   /* 4 bits, packable */
/* z: world units along track (>=0). x: lateral, FX_ONE = road edge.
   speed: world units/tick in fx. drift: ticks of qualifying full steer, 0..DRIFT_TICKS;
   drift == DRIFT_TICKS means drifting. zfrac: sub-unit remainder of z (1/256), so distance is the exact sum of speed. wall_hit: 1 for the tick of a new wall contact. */
typedef struct { int32_t z; fx x; fx speed; int8_t drift; uint8_t wall_hit; uint8_t zfrac; } car_t;

#define MAX_SPEED    FX(60)   /* tune in Task 6 */
#define MAX_REVERSE  FX(10)
#define ACCEL_SHIFT  6        /* accel = (MAX_SPEED - speed) >> this */
#define BRAKE_DECEL  500
#define REVERSE_ACCEL 100
#define COAST_SHIFT  7        /* coast drag = speed >> this, plus 1 */
#define STEER_RATE   38       /* x per tick at full speed */
#define CENTRIF_K    9        /* x push per tick = curve * speed_frac^2 * K */
#define DRIFT_TICKS  6
#define DRIFT_SPEED_PCT 75
#define DRIFT_STEER_PCT 105
#define DRIFT_BLEED_DIV 600   /* speed -= speed / this per drifting tick */
#define OFFROAD_PCT  35
#define OFFROAD_GRIP_PCT 50   /* steering grip on grass: going wide is costly, braking pays */
#define OFFROAD_SHIFT 3
#define WALL_SCRAPE_SHIFT 4  /* speed -= speed >> this per tick while pinned at the wall */
#define WALL_X       (FX_ONE * 3 / 2)
#define HILL_K       2        /* speed lost per unit of y rise to next seg (track y up = uphill) */

void car_reset(car_t *c);
void car_step(car_t *c, const track_t *t, uint8_t input);
#endif
