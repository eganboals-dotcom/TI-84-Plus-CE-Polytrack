#include <string.h>
#include "race.h"

void race_start(race_t *r, const track_t *t, const record_t *best)
{
    memset(r, 0, sizeof *r);
    r->track = t; r->best = best;
    car_reset(&r->car); car_reset(&r->ghost);
    r->run.n_cp = t->n_cp;
    r->last_split_cp = 0xFF;
}

uint8_t record_input(const record_t *rec, uint16_t tick)
{
    return (rec->inputs[tick >> 1] >> ((tick & 1) << 2)) & 15;
}

void race_tick(race_t *r, uint8_t input)
{
    if (r->finished || r->capped) return;
    input &= 15;
    if (r->tick & 1) r->run.inputs[r->tick >> 1] |= (uint8_t)(input << 4);
    else r->run.inputs[r->tick >> 1] = input;
    if (r->best && r->tick < r->best->ticks)
        car_step(&r->ghost, r->track, record_input(r->best, r->tick));
    car_step(&r->car, r->track, input);
    r->tick++;

    if (r->next_cp < r->track->n_cp && r->car.z >= (int32_t)r->track->cp_seg[r->next_cp] * SEG_LEN) {
        uint8_t i = r->next_cp++;
        r->run.splits[i] = r->tick;
        r->last_split_cp = i;
        r->last_split_delta = (r->best && r->best->ticks) ? (int16_t)(r->tick - r->best->splits[i]) : 0;
    }
    if (r->car.z >= track_length(r->track)) { r->finished = 1; r->run.ticks = r->tick; }
    else if (r->tick >= MAX_TICKS) r->capped = 1;
}

int race_is_new_best(const race_t *r)
{
    return r->finished && !r->capped && (r->best->ticks == 0 || r->run.ticks < r->best->ticks);
}

void format_time(char out[10], uint16_t ticks)
{
    uint32_t h = (uint32_t)ticks * 100 / TICKS_PER_SEC;
    uint32_t s = h / 100 % 60;
    out[0] = '0' + h / 6000; out[1] = ':';
    out[2] = '0' + s / 10; out[3] = '0' + s % 10; out[4] = '.';
    out[5] = '0' + h % 100 / 10; out[6] = '0' + h % 10; out[7] = 0;
}

void format_delta(char out[10], int16_t d)
{
    uint32_t h = (uint32_t)(d < 0 ? -d : d) * 100 / TICKS_PER_SEC, w = h / 100;
    uint8_t n = w >= 100 ? 3 : w >= 10 ? 2 : 1;   /* deltas reach 300 s */
    out[0] = d < 0 ? '-' : '+';
    out[n + 1] = '.';
    out[n + 2] = '0' + h % 100 / 10; out[n + 3] = '0' + h % 10; out[n + 4] = 0;
    for (; n; n--, w /= 10) out[n] = '0' + w % 10;
}
