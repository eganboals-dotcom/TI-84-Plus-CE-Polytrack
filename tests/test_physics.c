#include <stdlib.h>
#include "test.h"
#include "physics.h"

static track_t straight, curved;

static void mk(track_t *t, int8_t curve)
{
    t->n_segs = 1000; t->n_cp = 0;
    for (int i = 0; i < 1000; i++) { t->curve[i] = curve; t->y[i] = 0; }
}
static void setup(void) { mk(&straight, 0); mk(&curved, 6); }

void test_deterministic(void)
{
    setup();
    car_t a, b; car_reset(&a); car_reset(&b);
    uint32_t r = 1;
    for (int i = 0; i < 900; i++) {
        r = r * 1103515245u + 12345u;
        uint8_t in = (r >> 16) & 15;
        car_step(&a, &curved, in); car_step(&b, &curved, in);
        CHECK(a.z == b.z && a.x == b.x && a.speed == b.speed);
    }
}

void test_accel_and_topspeed(void)
{
    setup();
    car_t c; car_reset(&c);
    for (int i = 0; i < 600; i++) { car_step(&c, &straight, IN_GAS); CHECK(c.speed <= MAX_SPEED); }
    CHECK(c.speed >= MAX_SPEED * 98 / 100);
}

void test_brake_beats_coast(void)
{
    setup();
    car_t top, a, b, g;
    car_reset(&top); top.speed = MAX_SPEED; a = b = g = top;
    for (int i = 0; i < 30; i++) {
        car_step(&a, &straight, IN_BRAKE);
        car_step(&b, &straight, 0);
        car_step(&g, &straight, IN_BRAKE | IN_GAS);
    }
    CHECK(a.speed < b.speed);
    CHECK(g.speed == a.speed);
}

void test_reverse(void)
{
    setup();
    car_t c; car_reset(&c); c.z = 5000;
    int neg = 0;
    for (int i = 0; i < 300; i++) {
        car_step(&c, &straight, IN_BRAKE);
        if (c.speed < 0) neg = 1;
        CHECK(c.speed >= -MAX_REVERSE);
    }
    CHECK(neg);
    CHECK(c.speed == -MAX_REVERSE);
}

void test_no_reverse_past_start(void)
{
    setup();
    car_t c; car_reset(&c); c.z = 50;
    for (int i = 0; i < 300; i++) { car_step(&c, &straight, IN_BRAKE); CHECK(c.z >= 0); }
    CHECK(c.z == 0);
    CHECK(c.speed == 0);
}

void test_too_fast_goes_wide(void)
{
    setup();
    car_t c; car_reset(&c); c.speed = MAX_SPEED;
    int wide = 0;
    for (int i = 0; i < 90; i++) {
        car_step(&c, &curved, IN_GAS | IN_LEFT);   /* curve 6 is right; outside is left (x<0) */
        if (abs(c.x) > FX_ONE) wide = 1;
    }
    CHECK(wide);
}

void test_braked_entry_holds_line(void)
{
    setup();
    for (int dir = 0; dir < 2; dir++) {
        mk(&curved, dir ? -6 : 6);
        car_t c; car_reset(&c); c.speed = MAX_SPEED * 65 / 100;
        for (int i = 0; i < 150; i++) {
            uint8_t in = IN_GAS;
            /* outside is x<0 for right curve; steer inward when x drifts outward past 0.3 */
            if (dir == 0 && c.x < -FX_ONE * 3 / 10) in |= IN_RIGHT;
            if (dir == 1 && c.x > FX_ONE * 3 / 10) in |= IN_LEFT;
            car_step(&c, &curved, in);
            CHECK(abs(c.x) <= FX_ONE);
        }
    }
}

void test_wall_slows(void)
{
    setup();
    car_t c; car_reset(&c); c.speed = FX(40); c.x = FX_ONE * 2;
    car_step(&c, &straight, 0);
    CHECK(c.speed <= FX(20) + 64);
    CHECK(abs(c.x) <= FX_ONE * 3 / 2);
    CHECK(c.wall_hit == 1);
    car_step(&c, &straight, 0);
    CHECK(c.wall_hit == 0);
}
