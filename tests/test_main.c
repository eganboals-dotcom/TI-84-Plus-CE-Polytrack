#include <stdio.h>
#include "test.h"
int g_fail = 0;
void test_all_tracks_build(void);
void test_track_heights_bounded(void);
int main(void)
{
    test_all_tracks_build();
    test_track_heights_bounded();
    printf("%d failures\n", g_fail);
    return g_fail != 0;
}
