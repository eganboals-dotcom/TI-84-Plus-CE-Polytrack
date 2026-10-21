#ifndef TRACK_H
#define TRACK_H
#include <stdint.h>
#define SEG_LEN 200            /* world units per segment (fx-free ints) */
#define MAX_SEGS 1200
#define MAX_CP 8
typedef struct { uint8_t count; int8_t curve; int8_t hill; } section_t; /* curve -6..6 (right +), hill = height change per segment */
typedef struct { const char *name; const section_t *sections; uint8_t n_sections; uint8_t n_cp; uint8_t palette; } track_def_t;
typedef struct { int8_t curve[MAX_SEGS]; int16_t y[MAX_SEGS]; uint16_t n_segs; uint16_t cp_seg[MAX_CP]; uint8_t n_cp; } track_t;
extern const track_def_t TRACKS[]; extern const uint8_t N_TRACKS;
void track_build(track_t *t, const track_def_t *d);
int32_t track_length(const track_t *t);   /* n_segs * SEG_LEN */
#endif
