#include <string.h>
#include "savefmt.h"

static uint8_t sum(const uint8_t *p, size_t n)
{
    uint8_t s = 0;
    while (n--) s = (uint8_t)(s + *p++);
    return s;
}

size_t save_serialize(const record_t *rec, uint8_t *buf, size_t cap)
{
    uint8_t i;
    size_t ni = ((size_t)rec->ticks + 1) / 2, n = 5 + 2 * (size_t)rec->n_cp + ni + 1, o = 5;
    if (rec->ticks == 0 || rec->ticks > MAX_TICKS || rec->n_cp > MAX_CP || cap < n) return 0;
    buf[0] = SAVE_MAGIC; buf[1] = SAVE_VERSION; buf[2] = rec->n_cp;
    buf[3] = (uint8_t)rec->ticks; buf[4] = (uint8_t)(rec->ticks >> 8);
    for (i = 0; i < rec->n_cp; i++) { buf[o++] = (uint8_t)rec->splits[i]; buf[o++] = (uint8_t)(rec->splits[i] >> 8); }
    memcpy(buf + o, rec->inputs, ni);
    if (rec->ticks & 1) buf[o + ni - 1] &= 0x0F;   /* unused high nibble stays 0 */
    buf[n - 1] = sum(buf, n - 1);
    return n;
}

int save_parse(record_t *rec, const uint8_t *buf, size_t len, uint8_t expect_n_cp)
{
    uint8_t i;
    uint16_t ticks;
    size_t ni, o = 5;
    rec->ticks = 0;
    if (len < 6 || buf[0] != SAVE_MAGIC || buf[1] != SAVE_VERSION || buf[2] != expect_n_cp || expect_n_cp > MAX_CP) return 0;
    ticks = (uint16_t)(buf[3] | (buf[4] << 8));
    if (ticks == 0 || ticks > MAX_TICKS) return 0;
    ni = ((size_t)ticks + 1) / 2;
    if (len != 5 + 2 * (size_t)expect_n_cp + ni + 1 || buf[len - 1] != sum(buf, len - 1)) return 0;
    memset(rec, 0, sizeof *rec);
    for (i = 0; i < expect_n_cp; i++, o += 2) {
        rec->splits[i] = (uint16_t)(buf[o] | (buf[o + 1] << 8));
        if (rec->splits[i] >= ticks || (i && rec->splits[i] <= rec->splits[i - 1])) { rec->ticks = 0; return 0; }
    }
    memcpy(rec->inputs, buf + o, ni);
    rec->n_cp = expect_n_cp;
    rec->ticks = ticks;
    return 1;
}
