#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "fileioc.h"

int shim_fail_archive = 0, shim_fail_rename = 0;
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
    if (shim_fail_rename) return 2;
    path_of(a, sizeof a, o);
    path_of(b, sizeof b, n);
    if ((f = fopen(b, "rb"))) { fclose(f); return 1; }
    return rename(a, b) == 0 ? 0 : 2;
}

void ti_SetGCBehavior(void (*before)(void), void (*after)(void)) { (void)before; (void)after; }

/* ---- graphx ---- */
#include <assert.h>
#include "graphx.h"
#include "keypadc.h"

uint8_t kb_Data[8];
void (*shim_kb_hook)(void);
long shim_ticks;
void kb_Scan(void) { if (shim_kb_hook) shim_kb_hook(); }
long shim_clock(void) { return shim_ticks; }   /* same type as host clock_t */

static uint8_t vram[2][240][320];
static uint16_t pal[256];
static int vis = 0, to_buffer = 0;   /* visible vram index; draw target is the other one when to_buffer */
static uint8_t color, text_fg;
static int clip_x0 = 0, clip_y0 = 0, clip_x1 = 320, clip_y1 = 240;
#define DRAWBUF vram[to_buffer ? 1 - vis : vis]

void gfx_Begin(void) { vis = 0; to_buffer = 0; }
void gfx_End(void) {}
void gfx_SetDraw(uint8_t loc) { to_buffer = loc == gfx_buffer; }
void gfx_SwapDraw(void) { vis = 1 - vis; }
void gfx_SetPalette(const void *p, uint24_t size, uint8_t off) { memcpy(&pal[off], p, size); }
uint8_t gfx_SetColor(uint8_t i) { uint8_t o = color; color = i; return o; }
void gfx_FillScreen(uint8_t i) { memset(DRAWBUF, i, sizeof vram[0]); }

void gfx_HorizLine_NoClip(uint24_t x, uint8_t y, uint24_t len)
{
    assert(y < 240 && x + len <= 320);   /* NoClip out of range corrupts memory on CE */
    memset(&DRAWBUF[y][x], color, len);
}

void gfx_FillRectangle_NoClip(uint24_t x, uint8_t y, uint24_t w, uint8_t h)
{
    assert(x + w <= 320 && (unsigned)y + h <= 240);
    for (unsigned r = 0; r < h; r++) memset(&DRAWBUF[y + r][x], color, w);
}

void gfx_FillRectangle(int x, int y, int w, int h)
{
    int x1 = x + w, y1 = y + h;
    if (x < clip_x0) x = clip_x0;
    if (y < clip_y0) y = clip_y0;
    if (x1 > clip_x1) x1 = clip_x1;
    if (y1 > clip_y1) y1 = clip_y1;
    for (; y < y1; y++) if (x1 > x) memset(&DRAWBUF[y][x], color, x1 - x);
}

static long edge(int ax, int ay, int bx, int by, int px, int py) { return (long)(bx - ax) * (py - ay) - (long)(by - ay) * (px - ax); }

void gfx_FillTriangle(int x0, int y0, int x1, int y1, int x2, int y2)
{
    long area = edge(x0, y0, x1, y1, x2, y2);
    for (int y = clip_y0; y < clip_y1; y++)   /* host only: brute-force inside test */
        for (int x = clip_x0; x < clip_x1; x++) {
            long a = edge(x0, y0, x1, y1, x, y), b = edge(x1, y1, x2, y2, x, y), c = edge(x2, y2, x0, y0, x, y);
            if (area >= 0 ? (a >= 0 && b >= 0 && c >= 0) : (a <= 0 && b <= 0 && c <= 0)) DRAWBUF[y][x] = color;
        }
}

void gfx_SetClipRegion(int xmin, int ymin, int xmax, int ymax) { clip_x0 = xmin; clip_y0 = ymin; clip_x1 = xmax; clip_y1 = ymax; }

/* shortcut: no font in shim, text is a filled box 8*len x 8 in the fg color; add a font if text layout needs visual checking */
void gfx_PrintStringXY(const char *s, int x, int y)
{
    uint8_t c = color;
    color = text_fg;
    gfx_FillRectangle(x, y, 8 * (int)strlen(s), 8);
    color = c;
}
uint8_t gfx_SetTextFGColor(uint8_t c) { uint8_t o = text_fg; text_fg = c; return o; }
uint8_t gfx_SetTextBGColor(uint8_t c) { return c; }
uint8_t gfx_SetTextTransparentColor(uint8_t c) { return c; }
unsigned int gfx_GetStringWidth(const char *s) { return 8 * (unsigned)strlen(s); }

int shim_dump_ppm(const char *path)
{
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    fprintf(f, "P6\n320 240\n255\n");
    for (int y = 0; y < 240; y++)
        for (int x = 0; x < 320; x++) {
            uint16_t c = pal[vram[vis][y][x]];
            uint8_t rgb[3] = {(uint8_t)(((c >> 10) & 31) << 3), (uint8_t)(((c >> 5) & 31) << 3), (uint8_t)((c & 31) << 3)};
            fwrite(rgb, 1, 3, f);
        }
    return fclose(f) == 0;
}
