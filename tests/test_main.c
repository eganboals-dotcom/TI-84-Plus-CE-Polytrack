#include <stdio.h>
#include "test.h"
int g_fail = 0;
void test_all_tracks_build(void);
void test_track_heights_bounded(void);
void test_deterministic(void);
void test_accel_and_topspeed(void);
void test_brake_beats_coast(void);
void test_reverse(void);
void test_no_reverse_past_start(void);
void test_too_fast_goes_wide(void);
void test_braked_entry_holds_line(void);
void test_wall_slows(void);
void test_fractional_z(void);
void test_uphill_slows(void);
int main(void)
{
    test_all_tracks_build();
    test_track_heights_bounded();
    test_deterministic();
    test_accel_and_topspeed();
    test_brake_beats_coast();
    test_reverse();
    test_no_reverse_past_start();
    test_too_fast_goes_wide();
    test_braked_entry_holds_line();
    test_wall_slows();
    test_fractional_z();
    test_uphill_slows();
    printf("%d failures\n", g_fail);
    return g_fail != 0;
}
