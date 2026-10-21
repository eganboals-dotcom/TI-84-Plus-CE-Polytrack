#include <fileioc.h>
#include "save.h"
#include "savefmt.h"

static uint8_t buf[SAVE_MAX_BYTES];   /* static: CE stack is tiny */

static void name_of(char out[8], uint8_t idx, uint8_t tmp)
{
    const char *p = tmp ? "RCRTM0" : "RCRSV0";
    uint8_t i;
    for (i = 0; i < 7; i++) out[i] = p[i];
    out[5] = (char)('0' + idx);
}

static void load_file(const char *name, uint8_t n_cp, record_t *rec)
{
    uint8_t h = ti_Open(name, "r");
    uint16_t n;
    if (!h) return;
    n = ti_GetSize(h);
    if (n <= sizeof buf && ti_Read(buf, 1, n, h) == n) save_parse(rec, buf, n, n_cp);
    ti_Close(h);
}

void save_load(uint8_t track_idx, uint8_t n_cp, record_t *rec)
{
    char name[8];
    rec->ticks = 0;
    name_of(name, track_idx, 0);
    load_file(name, n_cp, rec);
    if (rec->ticks) return;
    name_of(name, track_idx, 1);   /* a failed swap leaves the new record in the temp file */
    load_file(name, n_cp, rec);
}

int save_store(uint8_t track_idx, const record_t *rec)
{
    char name[8], tmp[8];
    uint8_t h;
    size_t n = save_serialize(rec, buf, sizeof buf);
    if (!n) return 0;
    name_of(name, track_idx, 0);
    name_of(tmp, track_idx, 1);
    ti_Delete(tmp);
    h = ti_Open(tmp, "w");
    if (!h) return 0;
    if (ti_Write(buf, 1, n, h) != n || !ti_SetArchiveStatus(1, h)) {
        ti_Close(h);
        ti_Delete(tmp);
        return 0;
    }
    ti_Close(h);
    ti_Delete(name);
    return ti_Rename(tmp, name) == 0;   /* on failure keep tmp: save_load falls back to it */
}
