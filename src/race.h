#ifndef RACE_H
#define RACE_H
#include <stdint.h>
#include "physics.h"

#define TICKS_PER_SEC 30
#define MAX_TICKS (TICKS_PER_SEC * 60 * 5)          /* 5 min cap = 9000 ticks */
typedef struct { uint16_t ticks; uint16_t splits[MAX_CP]; uint8_t n_cp; uint8_t inputs[MAX_TICKS / 2]; } record_t; /* ticks==0 => no record */
typedef struct {
    car_t car, ghost; const track_t *track; const record_t *best;  /* best may have ticks==0 */
    record_t run; uint16_t tick; uint8_t next_cp; uint8_t finished; uint8_t capped;
    int16_t last_split_delta; uint8_t last_split_cp;  /* delta in ticks vs best, valid if last_split_cp != 0xFF */
} race_t;

void race_start(race_t *r, const track_t *t, const record_t *best);
void race_tick(race_t *r, uint8_t input);   /* no-op once finished or capped */
uint8_t record_input(const record_t *rec, uint16_t tick);
int race_is_new_best(const race_t *r);
void format_time(char out[10], uint16_t ticks);   /* "M:SS.hh" */
void format_delta(char out[10], int16_t dticks);  /* "+0.47" / "-1.20" */
#endif
