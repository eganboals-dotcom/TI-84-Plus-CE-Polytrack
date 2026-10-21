#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileioc.h"

int shim_fail_archive = 0;
static FILE *files[8];   /* handle h -> files[h-1] */

static void path_of(char *out, size_t cap, const char *name)
{
    const char *d = getenv("SHIM_DIR");
    snprintf(out, cap, "%s/%s", d ? d : "build/appvars", name);
}

uint8_t ti_Open(const char *name, const char *mode)
{
    char p[256], cmd[300];
    uint8_t h;
    FILE *f;
    for (h = 0; h < 8 && files[h]; h++) {}
    if (h == 8) return 0;
    path_of(p, sizeof p, name);
    if (mode[0] == 'w') {
        snprintf(cmd, sizeof cmd, "mkdir -p \"%s\"", getenv("SHIM_DIR") ? getenv("SHIM_DIR") : "build/appvars");
        if (system(cmd)) return 0;
    }
    f = fopen(p, mode[0] == 'w' ? "wb" : "rb");
    if (!f) return 0;
    files[h] = f;
    return (uint8_t)(h + 1);
}

int ti_Close(uint8_t h) { int r = fclose(files[h - 1]); files[h - 1] = NULL; return r; }
size_t ti_Write(const void *d, size_t s, size_t c, uint8_t h) { return fwrite(d, s, c, files[h - 1]); }
size_t ti_Read(void *d, size_t s, size_t c, uint8_t h) { return fread(d, s, c, files[h - 1]); }

uint16_t ti_GetSize(uint8_t h)
{
    FILE *f = files[h - 1];
    long cur = ftell(f), n;
    fseek(f, 0, SEEK_END);
    n = ftell(f);
    fseek(f, cur, SEEK_SET);
    return (uint16_t)n;
}

#undef ti_SetArchiveStatus
int ti_SetArchiveStatus(uint8_t archive, uint8_t h)
{
    (void)archive;
    if (shim_fail_archive) return 0;
    fflush(files[h - 1]);
    return 1;
}

int ti_Delete(const char *name) { char p[256]; path_of(p, sizeof p, name); return remove(p) == 0; }

uint8_t ti_Rename(const char *o, const char *n)
{
    char a[256], b[256];
    FILE *f;
    path_of(a, sizeof a, o);
    path_of(b, sizeof b, n);
    if ((f = fopen(b, "rb"))) { fclose(f); return 1; }
    return rename(a, b) == 0 ? 0 : 2;
}
