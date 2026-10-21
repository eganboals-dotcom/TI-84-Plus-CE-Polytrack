#ifndef SAVE_H
#define SAVE_H
#include <stdint.h>
#include "race.h"

/* Loads RCRSV<track_idx>; rec->ticks = 0 if missing/corrupt/n_cp mismatch. */
void save_load(uint8_t track_idx, uint8_t n_cp, record_t *rec);
/* Atomic-ish: temp file, archive, then swap. 1 ok, 0 failed (old record kept). */
int save_store(uint8_t track_idx, const record_t *rec);
#endif
