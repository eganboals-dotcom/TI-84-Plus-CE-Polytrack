#include "test.h"
#include "render.h"

void test_projection(void)
{
    int far = project_y(1000000, 0);
    CHECK(far >= HORIZON_Y && far <= HORIZON_Y + 2);
    int prev = project_y(300, 0);
    CHECK(prev > 240);                          /* road right under the camera is below the screen */
    for (int32_t dz = 600; dz <= (int32_t)DRAW_SEGS * 200; dz *= 2) {   /* doubling: far rows are < 1 px apart per segment */
        int y = project_y(dz, 0);
        CHECK(y < prev);
        prev = y;
    }
    CHECK(project_y(4000, 400) < project_y(4000, 0));   /* hill ahead projects above flat */
    CHECK(project_y(4000, -400) > project_y(4000, 0));
}
