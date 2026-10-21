/* Shared autopilot (drivable-track test + sim): brake when the sharpest curve within
   the next 15 segments times speed is over AP_THRESH, else gas; steer against the
   centrifugal push toward x = 0. */
#ifndef AUTOPILOT_H
#define AUTOPILOT_H
#include "physics.h"

#define AP_LOOK   15
#define AP_THRESH 262      /* |curve| * speed(units/tick); ~holdable limit 266/|curve| incl. drift */

static inline uint8_t autopilot(const car_t *c, const track_t *t)
{
    int32_t seg = c->z / SEG_LEN, k, cv = 0, cur;
    for (k = seg; k <= seg + AP_LOOK && k < t->n_segs; k++) {
        int32_t a = t->curve[k] < 0 ? -t->curve[k] : t->curve[k];
        if (a > cv) cv = a;
    }
    cur = seg < t->n_segs ? t->curve[seg] : 0;
    {
        int32_t sf = c->speed * FX_ONE / MAX_SPEED;
        fx next = c->x - ((cur * ((sf * sf) >> 8) * CENTRIF_K) >> 8);   /* x after this tick's push */
        uint8_t in = cv * (c->speed >> 8) > AP_THRESH ? IN_BRAKE : IN_GAS;
        if (next < -16) in |= IN_RIGHT;
        else if (next > 16) in |= IN_LEFT;
        return in;
    }
}
#endif
