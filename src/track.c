#include "track.h"

/* Placeholder layouts; Task 6 tunes them. */
static const section_t S0[] = {{100,0,0},{60,2,0},{80,0,2},{60,-2,-2},{60,0,0}};
static const section_t S1[] = {{60,0,3},{60,2,-3},{60,-2,3},{60,3,-3},{60,0,0},{60,-3,2}};
static const section_t S2[] = {{50,0,0},{40,5,0},{40,-5,0},{50,0,0},{40,6,0},{40,-6,0},{50,4,0},{50,0,0}};
static const section_t S3[] = {{50,3,2},{50,-4,-2},{50,5,3},{50,-5,-3},{50,2,2},{50,-3,-2},{50,0,0},{50,4,0}};
static const section_t S4[] = {{40,4,4},{40,-4,-4},{40,5,4},{40,-5,-4},{40,6,3},{40,-6,-3},{40,0,0},{40,4,2},{40,-4,-2},{40,0,0}};
static const section_t S5[] = {{40,5,4},{40,-6,-4},{40,6,5},{40,-6,-5},{40,4,4},{40,-5,-4},{40,6,3},{40,-6,-3},{40,5,0},{40,-5,0},{40,3,3},{40,-3,-3}};

#define D(n, s, cp, pal) { n, s, (uint8_t)(sizeof(s) / sizeof(s[0])), cp, pal }
const track_def_t TRACKS[] = {
    D("Warmup", S0, 2, 0),
    D("Rolling Hills", S1, 3, 1),
    D("Hairpin Alley", S2, 3, 2),
    D("Canyon Run", S3, 4, 3),
    D("Twin Peaks", S4, 4, 4),
    D("The Gauntlet", S5, 5, 5),
};
const uint8_t N_TRACKS = 6;

void track_build(track_t *t, const track_def_t *d)
{
    uint16_t n = 0;
    int16_t y0 = 0;
    for (uint8_t s = 0; s < d->n_sections; s++) {
        const section_t *sec = &d->sections[s];
        int16_t total = (int16_t)(sec->hill * sec->count);
        for (uint8_t k = 0; k < sec->count && n < MAX_SEGS; k++, n++) {
            int32_t u = (int32_t)k * 256 / sec->count;       /* 0..255 */
            int32_t sm = (u * u * (768 - 2 * u)) >> 16;      /* smoothstep, 0..256 */
            t->curve[n] = sec->curve;
            t->y[n] = (int16_t)(y0 + ((total * sm) >> 8));
        }
        y0 = (int16_t)(y0 + total);
    }
    t->n_segs = n;
    t->n_cp = d->n_cp;
    for (uint8_t i = 0; i < d->n_cp && i < MAX_CP; i++)
        t->cp_seg[i] = (uint16_t)(n * (i + 1) / (d->n_cp + 1));
}

int32_t track_length(const track_t *t) { return (int32_t)t->n_segs * SEG_LEN; }
