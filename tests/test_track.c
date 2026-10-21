#include <stdlib.h>
#include "test.h"
#include "track.h"

void test_all_tracks_build(void)
{
    static track_t t;
    CHECK(N_TRACKS == 6);
    for (int i = 0; i < N_TRACKS; i++) {
        track_build(&t, &TRACKS[i]);
        CHECK(t.n_segs >= 300 && t.n_segs <= MAX_SEGS);
        CHECK(t.n_cp >= 2 && t.n_cp <= MAX_CP);
        CHECK(track_length(&t) == (int32_t)t.n_segs * SEG_LEN);
        for (int c = 0; c < t.n_cp; c++) {
            CHECK(t.cp_seg[c] < t.n_segs);
            if (c) CHECK(t.cp_seg[c] > t.cp_seg[c - 1]);
        }
        for (int s = 0; s < t.n_segs; s++) CHECK(t.curve[s] >= -6 && t.curve[s] <= 6);
    }
}

void test_track_heights_bounded(void)
{
    static track_t t;
    for (int i = 0; i < N_TRACKS; i++) {
        track_build(&t, &TRACKS[i]);
        for (int s = 0; s < t.n_segs; s++) CHECK(abs(t.y[s]) < 4000);
    }
}
