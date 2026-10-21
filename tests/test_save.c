#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "test.h"
#include "savefmt.h"
#include "save.h"
#include "fileioc.h"

static record_t a, b, c;
static uint8_t buf[SAVE_MAX_BYTES], bad[SAVE_MAX_BYTES];

static void fill(record_t *r, uint16_t ticks, uint8_t seed)
{
    uint16_t i;
    memset(r, 0, sizeof *r);
    srand(seed);
    r->ticks = ticks; r->n_cp = 3;
    r->splits[0] = ticks / 4; r->splits[1] = ticks / 2; r->splits[2] = (uint16_t)(ticks * 3 / 4);
    for (i = 0; i < (ticks + 1) / 2; i++) r->inputs[i] = (uint8_t)(rand() & 0xFF);
    if (ticks & 1) r->inputs[ticks / 2] &= 0x0F;
}

static int same(const record_t *x, const record_t *y)
{
    return x->ticks == y->ticks && x->n_cp == y->n_cp && !memcmp(x->splits, y->splits, sizeof x->splits)
        && !memcmp(x->inputs, y->inputs, (x->ticks + 1) / 2);
}

void test_roundtrip(void)
{
    size_t n;
    fill(&a, 1234, 1);
    n = save_serialize(&a, buf, sizeof buf);
    CHECK(n == 5 + 6 + 617 + 1);
    CHECK(save_serialize(&a, buf, n - 1) == 0);
    CHECK(save_parse(&c, buf, n, 3) == 1);
    CHECK(same(&a, &c));
    fill(&a, 777, 2);   /* odd ticks */
    n = save_serialize(&a, buf, sizeof buf);
    CHECK(save_parse(&c, buf, n, 3) == 1 && same(&a, &c));
}

void test_load_rejects_corrupt(void)
{
    size_t n, i;
    fill(&a, 1234, 3);
    n = save_serialize(&a, buf, sizeof buf);
    for (i = 0; i < n; i++) {
        memcpy(bad, buf, n);
        bad[i] ^= 0x5A;
        c.ticks = 99;
        CHECK(save_parse(&c, bad, n, 3) == 0 && c.ticks == 0);
    }
    c.ticks = 99;
    CHECK(save_parse(&c, buf, n - 1, 3) == 0 && c.ticks == 0);
    memcpy(bad, buf, n);
    bad[1] = 2; bad[n - 1] = (uint8_t)(bad[n - 1] + 1);   /* version 2 with a valid checksum */
    CHECK(save_parse(&c, bad, n, 3) == 0 && c.ticks == 0);
    CHECK(save_parse(&c, buf, n, 4) == 0 && c.ticks == 0);   /* wrong n_cp */

    /* files: garbage, missing */
    CHECK(system("mkdir -p build/appvars && echo garbage > build/appvars/RCRSV4") == 0);
    c.ticks = 99;
    save_load(4, 3, &c);
    CHECK(c.ticks == 0);
    remove("build/appvars/RCRSV5");
    c.ticks = 99;
    save_load(5, 3, &c);
    CHECK(c.ticks == 0);
}

void test_save_keeps_old_on_failure(void)
{
    fill(&a, 1000, 4);
    fill(&b, 900, 5);
    CHECK(save_store(0, &a) == 1);
    save_load(0, 3, &c);
    CHECK(same(&a, &c));
    shim_fail_archive = 1;
    CHECK(save_store(0, &b) == 0);
    shim_fail_archive = 0;
    save_load(0, 3, &c);
    CHECK(same(&a, &c));
    CHECK(save_store(0, &b) == 1);
    save_load(0, 3, &c);
    CHECK(same(&b, &c));
}

void test_save_rename_failure(void)
{
    FILE *f;
    fill(&a, 1000, 6);
    fill(&b, 900, 7);
    fill(&c, 800, 8);
    CHECK(save_store(1, &a) == 1);
    shim_fail_rename = 1;
    CHECK(save_store(1, &b) == 0);
    shim_fail_rename = 0;
    memset(&bad, 0, sizeof bad);
    {
        static record_t r;
        save_load(1, 3, &r);
        CHECK(r.ticks != 0 && (same(&a, &r) || same(&b, &r)));
    }
    CHECK(save_store(1, &c) == 1);
    {
        static record_t r;
        save_load(1, 3, &r);
        CHECK(same(&c, &r));
    }
    f = fopen("build/appvars/RCRTM1", "rb");
    CHECK(f == NULL);
    if (f) fclose(f);
}
