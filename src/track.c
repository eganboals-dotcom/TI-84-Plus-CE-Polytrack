#include "track.h"

/* Final layouts {segments, curve, hill/segment}. Difficulty climbs: gentle sweeps -> crests -> hairpins -> mixed. */
static const section_t S0[] = {{50,0,0},{30,1,0},{20,0,0},{30,-2,0},{25,0,2},{25,0,-2},{35,3,0},{20,0,0},{35,-3,0},{40,0,0}};
static const section_t S1[] = {{30,0,0},{25,0,4},{25,0,-4},{20,0,0},{30,2,3},{30,-2,-3},{20,0,5},{15,0,-6},{20,3,1},{20,0,-1},
                               {25,-3,4},{25,0,-4},{30,4,0},{35,0,0}};
static const section_t S2[] = {{30,0,0},{20,5,0},{10,0,0},{20,-6,0},{20,0,0},{15,6,0},{15,-6,0},{20,0,0},{20,3,0},{15,-5,0},
                               {20,0,0},{20,6,0},{10,-4,0},{30,0,0},{15,-6,0},{20,0,0}};
static const section_t S3[] = {{25,0,0},{25,3,3},{20,-4,-2},{15,0,4},{15,5,-4},{20,-5,0},{25,0,-3},{20,4,3},{20,-6,0},{15,0,5},
                               {15,0,-5},{25,5,2},{25,-3,-2},{20,6,0},{20,0,0},{15,-5,0},{25,0,0}};
static const section_t S4[] = {{25,0,0},{35,2,5},{15,-5,0},{25,0,-6},{15,4,0},{15,-4,0},{25,0,0},{35,-2,5},{15,6,0},{25,0,-6},
                               {15,-6,0},{20,5,2},{20,-5,-2},{25,3,0},{15,-6,0},{25,0,0},{35,0,0}};
static const section_t S5[] = {{20,0,0},{15,5,3},{15,-6,-3},{15,6,0},{20,0,5},{10,-5,-6},{20,4,0},{15,-6,2},{15,6,-2},{20,0,0},
                               {15,-5,5},{15,5,-5},{20,-6,0},{10,0,6},{10,0,-6},{15,6,0},{15,-6,0},{20,3,3},{15,-5,-3},{15,5,0},
                               {15,-6,0},{15,0,0},{20,6,0},{20,0,0}};

#define D(n, s, cp, pal) { n, s, (uint8_t)(sizeof(s) / sizeof(s[0])), cp, pal }
const track_def_t TRACKS[TRACK_COUNT] = {
    D("Warmup", S0, 2, 0),
    D("Rolling Hills", S1, 3, 1),
    D("Hairpin Alley", S2, 3, 2),
    D("Canyon Run", S3, 4, 3),
    D("Twin Peaks", S4, 4, 4),
    D("The Gauntlet", S5, 5, 5),
};
const uint8_t N_TRACKS = TRACK_COUNT;

void track_build(track_t *t, const track_def_t *d)
{
    uint16_t n = 0;
    int16_t y0 = 0;
    for (uint8_t s = 0; s < d->n_sections; s++) {
        const section_t *sec = &d->sections[s];
        int16_t total = (int16_t)(sec->hill * sec->count);
        for (uint8_t k = 0; k < sec->count && n < MAX_SEGS; k++, n++) {
            int32_t u = (int32_t)(k + 1) * 256 / sec->count; /* last segment reaches 256: no step at the boundary */
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
