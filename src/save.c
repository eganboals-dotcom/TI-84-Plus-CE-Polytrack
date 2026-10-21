#include <fileioc.h>
#include "save.h"
#include "savefmt.h"

static uint8_t buf[SAVE_MAX_BYTES];   /* static: CE stack is tiny */

static void name_of(char out[8], uint8_t idx)
{
    const char *p = "RCRSV0";
    uint8_t i;
    for (i = 0; i < 7; i++) out[i] = p[i];
    out[5] = (char)('0' + idx);
}

void save_load(uint8_t track_idx, uint8_t n_cp, record_t *rec)
{
    char name[8];
    uint8_t h;
    uint16_t n;
    rec->ticks = 0;
    name_of(name, track_idx);
    h = ti_Open(name, "r");
    if (!h) return;
    n = ti_GetSize(h);
    if (n <= sizeof buf && ti_Read(buf, 1, n, h) == n) save_parse(rec, buf, n, n_cp);
    ti_Close(h);
}

int save_store(uint8_t track_idx, const record_t *rec)
{
    char name[8];
    uint8_t h;
    size_t n = save_serialize(rec, buf, sizeof buf);
    if (!n) return 0;
    name_of(name, track_idx);
    ti_Delete("RCRTMP");
    h = ti_Open("RCRTMP", "w");
    if (!h) return 0;
    if (ti_Write(buf, 1, n, h) != n || !ti_SetArchiveStatus(1, h)) {
        ti_Close(h);
        ti_Delete("RCRTMP");
        return 0;
    }
    ti_Close(h);
    ti_Delete(name);
    if (ti_Rename("RCRTMP", name) != 0) { ti_Delete("RCRTMP"); return 0; }
    return 1;
}
