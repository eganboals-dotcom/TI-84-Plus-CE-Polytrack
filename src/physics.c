#include "physics.h"

void car_reset(car_t *c)
{
    c->z = 0; c->x = 0; c->speed = 0; c->drift = 0; c->wall_hit = 0; c->zfrac = 0;
}

void car_step(car_t *c, const track_t *t, uint8_t input)
{
    int32_t seg = c->z / SEG_LEN;
    if (seg > (int32_t)t->n_segs - 1) seg = t->n_segs - 1;
    int32_t curve = t->curve[seg];
    int32_t slope = t->y[seg < (int32_t)t->n_segs - 1 ? seg + 1 : seg] - t->y[seg];
    fx prev_x = c->x;
    fx s = c->speed;

    /* longitudinal */
    if (input & IN_BRAKE) {
        if (s > 0) { s -= BRAKE_DECEL; if (s < 0) s = 0; }
        else { s -= REVERSE_ACCEL; if (s < -MAX_REVERSE) s = -MAX_REVERSE; }
    } else if (input & IN_GAS) {
        s += (MAX_SPEED - s) >> ACCEL_SHIFT;
    } else if (s > 0) {
        s -= (s >> COAST_SHIFT) + 1;
        if (s < 0) s = 0;
    } else if (s < 0) {
        s += 100; if (s > 0) s = 0;
    }
    s -= slope * HILL_K;

    /* speed fraction, 0..256 */
    int32_t sf = s * FX_ONE / MAX_SPEED;
    int32_t sf2 = (sf * sf) >> 8;
    int32_t steer = ((input & IN_RIGHT) ? 1 : 0) - ((input & IN_LEFT) ? 1 : 0);

    if (steer && sf * 100 > DRIFT_SPEED_PCT * FX_ONE) {
        if (c->drift < DRIFT_TICKS) c->drift++;
    } else c->drift = 0;
    int drifting = c->drift >= DRIFT_TICKS;
    if (drifting) s -= s / DRIFT_BLEED_DIV;

    int32_t dx = steer * STEER_RATE * sf / FX_ONE;
    if (drifting) dx = dx * DRIFT_STEER_PCT / 100;
    if (prev_x > FX_ONE || prev_x < -FX_ONE) dx = dx * OFFROAD_GRIP_PCT / 100;   /* grass: little grip to steer back */
    c->x += dx - ((curve * sf2 * CENTRIF_K) >> 8);

    /* off-road */
    if (c->x > FX_ONE || c->x < -FX_ONE) {
        fx cap = MAX_SPEED * OFFROAD_PCT / 100;
        if (s > cap) { fx d = (s - cap) >> OFFROAD_SHIFT; s -= d ? d : 1; }
    }

    /* wall: halve once per contact (resting exactly on the clamp is the same contact) */
    c->wall_hit = 0;
    if (c->x > WALL_X || c->x < -WALL_X) {
        if (prev_x != WALL_X && prev_x != -WALL_X) { s /= 2; c->wall_hit = 1; }
        else s -= s >> WALL_SCRAPE_SHIFT;   /* pinned against the wall: scrape every tick */
        c->x = c->x > 0 ? WALL_X : -WALL_X;
    }

    if (s > MAX_SPEED) s = MAX_SPEED;
    if (s < -MAX_REVERSE) s = -MAX_REVERSE;
    c->speed = s;
    int32_t acc = (int32_t)c->zfrac + s;     /* floor division keeps the remainder exact */
    c->z += acc >> 8;
    c->zfrac = (uint8_t)(acc & 255);
    if (c->z < 0 || (c->z == 0 && s < 0)) { c->z = 0; c->zfrac = 0; if (s < 0) c->speed = 0; }
}
