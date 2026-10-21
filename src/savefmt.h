#ifndef SAVEFMT_H
#define SAVEFMT_H
#include <stddef.h>
#include <stdint.h>
#include "race.h"

#define SAVE_MAGIC 0x52   /* 'R' */
#define SAVE_VERSION 1
#define SAVE_MAX_BYTES (5 + 2 * MAX_CP + MAX_TICKS / 2 + 1)

size_t save_serialize(const record_t *rec, uint8_t *buf, size_t cap);   /* bytes written, 0 if cap too small */
int save_parse(record_t *rec, const uint8_t *buf, size_t len, uint8_t expect_n_cp); /* 1 ok, 0 invalid -> rec->ticks = 0 */
#endif
