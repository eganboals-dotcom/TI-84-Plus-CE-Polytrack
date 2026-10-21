/* Pseudo-3D road: segments projected near-to-far with a running clip_y, each
   filled as one set of horizontal spans per scanline (no pixel loops in C). */
#include <graphx.h>
#include <string.h>
#include "render.h"

#define FOCAL     200      /* px per world unit at dz = 1 */
#define PSHIFT    14       /* projection scale fixed-point bits */
#define NEAR_DZ   128      /* clamp so near products stay inside int32 */
#define ROAD_W    1000     /* road half width (world); car x FX_ONE = edge */
#define CAM_BACK  1700     /* camera distance behind car: car base lands at CAR_Y */
#define CAR_Y     227
#define CURVE_K   2        /* Lou's dx += curve * K per segment */
#define Y_SCALE   8        /* track heights are small; exaggerate for visible crests */
#define SKY_BAND  (HORIZON_Y - 50)
#define W_REF     ((ROAD_W * FOCAL) / CAM_BACK)   /* road half width at the car, px */

enum { C_SKY1 = 1, C_SKY2, C_MOUNT, C_FAR, C_GRASS1, C_GRASS2, C_RUM1, C_RUM2, C_ROAD1, C_ROAD2,
       C_LANE, C_GATE, C_BLACK, C_WHITE, C_POST, C_BODY, C_TIRE, C_BUMP, C_LIGHT, C_GLASS,
       C_GHOST, C_GHOSTD, C_HUD, C_GREEN, C_RED, C_N, C_TRANS = 255 };

static const uint16_t PAL[C_N] = {
    0,
    gfx_RGBTo1555(40, 96, 216), gfx_RGBTo1555(120, 176, 248), gfx_RGBTo1555(88, 104, 152), gfx_RGBTo1555(48, 112, 48),
    gfx_RGBTo1555(64, 168, 64), gfx_RGBTo1555(48, 144, 48), gfx_RGBTo1555(224, 32, 32), gfx_RGBTo1555(248, 248, 248),
    gfx_RGBTo1555(104, 104, 112), gfx_RGBTo1555(96, 96, 104), gfx_RGBTo1555(240, 240, 240), gfx_RGBTo1555(248, 208, 0),
    gfx_RGBTo1555(0, 0, 0), gfx_RGBTo1555(255, 255, 255), gfx_RGBTo1555(232, 232, 216), gfx_RGBTo1555(208, 24, 24),
    gfx_RGBTo1555(24, 24, 24), gfx_RGBTo1555(64, 64, 64), gfx_RGBTo1555(255, 160, 64), gfx_RGBTo1555(40, 56, 96),
    gfx_RGBTo1555(136, 160, 200), gfx_RGBTo1555(72, 88, 120), gfx_RGBTo1555(16, 16, 40), gfx_RGBTo1555(32, 224, 64),
    gfx_RGBTo1555(248, 48, 48),
};
/* tire, body, bumper, light, glass */
static const uint8_t CAR_COL[5] = { C_TIRE, C_BODY, C_BUMP, C_LIGHT, C_GLASS };
static const uint8_t GHOST_COL[5] = { C_GHOSTD, C_GHOST, C_GHOSTD, C_GHOST, C_GHOSTD };
static const int16_t MOUNTS[][3] = { {40, 60, 26}, {130, 40, 16}, {205, 72, 36}, {292, 48, 22} };   /* x, half width, height */

static int32_t recip[256];             /* 65536 / h: per-scanline steps without a division */
static int32_t sky_scroll;
static fx prev_x;
static uint8_t flash;
static int16_t px[DRAW_SEGS + 2], py[DRAW_SEGS + 2], pw[DRAW_SEGS + 2], pclip[DRAW_SEGS + 2];
static int32_t xw[DRAW_SEGS + 2];

static int clampi(int32_t v, int lim) { return v < -lim ? -lim : v > lim ? lim : (int)v; }
static int32_t pscale(int32_t dz) { return ((int32_t)FOCAL << PSHIFT) / (dz < NEAR_DZ ? NEAR_DZ : dz); }
static int proj_y(int32_t sc, int32_t dy) { return clampi(HORIZON_Y + (((CAM_H - dy) * sc) >> PSHIFT), 2000); }
int project_y(int32_t dz, int32_t dy) { return proj_y(pscale(dz), dy); }

static int32_t seg_y(const track_t *t, int32_t s) { return t->y[s < 0 ? 0 : s >= t->n_segs ? t->n_segs - 1 : s]; }
static int seg_curve(const track_t *t, int32_t s) { return (s < 0 || s >= t->n_segs) ? 0 : t->curve[s]; }

void render_init(void)
{
    gfx_SetPalette(PAL, sizeof PAL, 0);
    gfx_SetDrawBuffer();
    gfx_SetTextBGColor(C_TRANS);
    gfx_SetTextTransparentColor(C_TRANS);
    for (int h = 1; h < 256; h++) recip[h] = 65536L / h;
    sky_scroll = 0; prev_x = 0; flash = 0;
}

static void span(int x0, int x1, uint8_t y)
{
    if (x0 < 0) x0 = 0;
    if (x1 > GFX_LCD_WIDTH) x1 = GFX_LCD_WIDTH;
    if (x1 > x0) gfx_HorizLine_NoClip(x0, y, x1 - x0);
}

/* car shape in 1/256 scale units, base center (cx, by); clipped routines so far/occluded ghosts are safe */
static void car_rect(int cx, int by, int s, int x0, int y0, int x1, int y1)
{
    int a = cx + ((x0 * s) >> 8), b = cx + ((x1 * s) >> 8), c = by + ((y0 * s) >> 8), d = by + ((y1 * s) >> 8);
    if (b > a && d > c) gfx_FillRectangle(a, c, b - a, d - c);
}
static void draw_car(int cx, int by, int s, int lean, const uint8_t *col)
{
    int l = lean;
    gfx_SetColor(col[0]);
    car_rect(cx, by, s, -28, -12, -18, 0);
    car_rect(cx, by, s, 18, -12, 28, 0);
    gfx_SetColor(col[1]);
    car_rect(cx, by, s, -26, -19, 26, -4);
    car_rect(cx, by, s, -17 + l, -29, 17 + l, -25);
    gfx_FillTriangle(cx + (((-23 + l) * s) >> 8), by + ((-19 * s) >> 8), cx + (((-17 + l) * s) >> 8), by + ((-29 * s) >> 8),
                     cx + (((-17 + l) * s) >> 8), by + ((-19 * s) >> 8));
    gfx_FillTriangle(cx + (((23 + l) * s) >> 8), by + ((-19 * s) >> 8), cx + (((17 + l) * s) >> 8), by + ((-29 * s) >> 8),
                     cx + (((17 + l) * s) >> 8), by + ((-19 * s) >> 8));
    gfx_SetColor(col[4]);
    car_rect(cx, by, s, -15 + l, -25, 15 + l, -19);
    gfx_SetColor(col[2]);
    car_rect(cx, by, s, -22, -7, 22, -3);
    gfx_SetColor(col[3]);
    car_rect(cx, by, s, -24, -16, -15, -11);
    car_rect(cx, by, s, 15, -16, 24, -11);
}

static void hud_text(const char *s, int x, int y, uint8_t fg)
{
    int w = (int)gfx_GetStringWidth(s);
    if (x < 0) x = GFX_LCD_WIDTH - 4 - (w + 6);    /* negative x: right-align */
    gfx_SetColor(C_HUD);
    gfx_FillRectangle_NoClip(x, y, w + 6, 12);
    gfx_SetTextFGColor(fg);
    gfx_PrintStringXY(s, x + 3, y + 2);
}

static char *u2s(char *p, unsigned v)   /* writes digits + NUL, returns end */
{
    char tmp[8]; int n = 0;
    do { tmp[n++] = (char)('0' + v % 10); v /= 10; } while (v);
    while (n) *p++ = tmp[--n];
    *p = 0;
    return p;
}

static void draw_race(const race_t *r, uint8_t show_fps, uint8_t fps)
{
    const track_t *t = r->track;
    const car_t *c = &r->car;
    int32_t cam_z = c->z - CAM_BACK;
    int32_t b = cam_z >= 0 ? cam_z / SEG_LEN : -((SEG_LEN - 1 - cam_z) / SEG_LEN);
    int rem = (int)(cam_z - b * SEG_LEN);          /* camera offset into segment b, 0..SEG_LEN-1 */
    int32_t cs = c->z / SEG_LEN;
    int32_t road_y = seg_y(t, cs) + (seg_y(t, cs + 1) - seg_y(t, cs)) * (int32_t)(c->z - cs * SEG_LEN) / SEG_LEN;
    int clip = GFX_LCD_HEIGHT;
    int n;

    /* Lou's curves: lateral world offset of each boundary, relative to the camera's heading */
    {
        int32_t x = 0, dx = -(int32_t)seg_curve(t, b) * CURVE_K * rem / SEG_LEN;
        for (n = 0; n <= DRAW_SEGS; n++) { xw[n] = x; x += dx; dx += seg_curve(t, b + n) * CURVE_K; }
    }
    {
        int nc = (CAM_BACK + rem) / SEG_LEN, f = (CAM_BACK + rem) % SEG_LEN;
        int32_t shift = xw[nc] + (xw[nc + 1] - xw[nc]) * f / SEG_LEN + ((c->x * ROAD_W) >> 8);   /* camera sits over the car */
        for (n = 0; n <= DRAW_SEGS; n++) {
            int32_t sc = pscale((int32_t)n * SEG_LEN - rem);
            px[n] = (int16_t)(GFX_LCD_WIDTH / 2 + clampi(((xw[n] - shift) * sc) >> PSHIFT, 4000));
            py[n] = (int16_t)proj_y(sc, (seg_y(t, b + n) - road_y) * Y_SCALE);
            pw[n] = (int16_t)clampi((ROAD_W * sc) >> PSHIFT, 4000);
        }
    }

    int ph3 = (int)((b + 3000) % 3);                   /* stripe phase: alt flips every 3 segments, no per-segment division */
    uint8_t alt = (uint8_t)(((b + 3000) / 3) & 1);
    for (n = 0; n < DRAW_SEGS && clip > 0; n++) {
        int32_t s = b + n;
        int y1 = py[n], y2 = py[n + 1], h, top, row;
        int32_t inv, dx8, dw8, x8, w8;
        uint8_t gate = 0, fin = s == t->n_segs - 1;
        if (n && ++ph3 == 3) { ph3 = 0; alt ^= 1; }
        pclip[n] = (int16_t)clip;
        if (y2 >= clip || y2 >= y1) continue;       /* hidden behind nearer road, below screen, or facing away (h <= 0) */
        for (int i = 0; i < t->n_cp; i++) gate |= s == t->cp_seg[i];
        h = y1 - y2;
        inv = h < 256 ? recip[h] : 65536L / h;
        dx8 = ((int32_t)(px[n + 1] - px[n]) * inv) >> 8;
        dw8 = ((int32_t)(pw[n + 1] - pw[n]) * inv) >> 8;
        x8 = ((int32_t)px[n] << 8) + dx8 * (y1 - clip + 1);
        w8 = ((int32_t)pw[n] << 8) + dw8 * (y1 - clip + 1);
        top = y2 < 0 ? 0 : y2;
        for (row = clip - 1; row >= top; row--, x8 += dx8, w8 += dw8) {
            int xc = (int)(x8 >> 8), w = (int)(w8 >> 8), rw = (w >> 3) + 1;
            uint8_t y = (uint8_t)row;
            gfx_SetColor(alt ? C_GRASS1 : C_GRASS2);
            span(0, xc - w - rw, y);
            span(xc + w + rw, GFX_LCD_WIDTH, y);
            gfx_SetColor(alt ? C_RUM1 : C_RUM2);
            span(xc - w - rw, xc - w, y);
            span(xc + w, xc + w + rw, y);
            if (fin) {                              /* checkered: 8 squares, 2 rows of them */
                int q = w >> 2, k;
                uint8_t ph = (uint8_t)((y1 - row) * 2 < h);
                for (k = 0; k < 8; k++) {
                    gfx_SetColor(((k + ph) & 1) ? C_WHITE : C_BLACK);
                    span(xc - w + k * q, k == 7 ? xc + w : xc - w + (k + 1) * q, y);
                }
            } else if (gate) {
                gfx_SetColor(C_GATE);
                span(xc - w, xc + w, y);
            } else if (alt) {
                int lw = (w >> 5) + 1;
                gfx_SetColor(C_ROAD1);
                span(xc - w, xc - lw, y);
                span(xc + lw, xc + w, y);
                gfx_SetColor(C_LANE);
                span(xc - lw, xc + lw, y);
            } else {
                gfx_SetColor(C_ROAD2);
                span(xc - w, xc + w, y);
            }
        }
        clip = top;
    }
    for (; n <= DRAW_SEGS; n++) pclip[n] = (int16_t)clip;

    sky_scroll += (int32_t)seg_curve(t, cs) * (c->speed >> 8) * 2;
    /* sky + distant ground only in rows the road left unpainted */
    if (clip > 0) {
        int a = clip < SKY_BAND ? clip : SKY_BAND, hz = clip < HORIZON_Y ? clip : HORIZON_Y;
        int off;
        gfx_SetColor(C_SKY1);
        gfx_FillRectangle_NoClip(0, 0, GFX_LCD_WIDTH, a);
        if (hz > SKY_BAND) { gfx_SetColor(C_SKY2); gfx_FillRectangle_NoClip(0, SKY_BAND, GFX_LCD_WIDTH, hz - SKY_BAND); }
        if (clip > HORIZON_Y) { gfx_SetColor(C_FAR); gfx_FillRectangle_NoClip(0, HORIZON_Y, GFX_LCD_WIDTH, clip - HORIZON_Y); }
        off = (int)(((sky_scroll >> 8) % GFX_LCD_WIDTH + GFX_LCD_WIDTH) % GFX_LCD_WIDTH);
        if (hz > HORIZON_Y - 36) {
            gfx_SetClipRegion(0, 0, GFX_LCD_WIDTH, hz);
            gfx_SetColor(C_MOUNT);
            for (unsigned i = 0; i < sizeof MOUNTS / sizeof MOUNTS[0]; i++)
                for (int rep = 0; rep < 2; rep++) {
                    int mx = MOUNTS[i][0] - off + rep * GFX_LCD_WIDTH;
                    gfx_FillTriangle(mx - MOUNTS[i][1], HORIZON_Y, mx, HORIZON_Y - MOUNTS[i][2], mx + MOUNTS[i][1], HORIZON_Y);
                }
        }
    }

    /* sprites far-to-near, each clipped above the nearer road it stands behind */
    {
        int32_t gdz = r->ghost.z - cam_z + rem;
        int gn = -1, gf = 0;
        if (r->best && r->best->ticks && r->ghost.z > c->z && gdz < (int32_t)DRAW_SEGS * SEG_LEN) {
            gn = (int)(gdz / SEG_LEN);
            gf = (int)(gdz % SEG_LEN);
        }
        for (n = DRAW_SEGS; n >= 0; n--) {
            int32_t s = b + n;
            int w = pw[n], xc = px[n], y = py[n], gate = 0;
            for (int i = 0; i < t->n_cp; i++) gate |= s == t->cp_seg[i];
            if (!(gate || s == t->n_segs - 1 || ((s & 7) == 0 && s >= 0) || n == gn)) continue;
            if (pclip[n] <= 0 || y - 2 * w >= pclip[n]) continue;
            gfx_SetClipRegion(0, 0, GFX_LCD_WIDTH, pclip[n]);
            if (gate || s == t->n_segs - 1) {       /* gate: pillars + bar over the road */
                int pwid = (w >> 4) + 1, ph = (w * 5) >> 2, bar = (w >> 3) + 1, k;   /* taller than CAM_H: bar passes over the camera */
                gfx_SetColor(C_POST);
                gfx_FillRectangle(xc - w - pwid * 2, y - ph, pwid, ph);
                gfx_FillRectangle(xc + w + pwid, y - ph, pwid, ph);
                if (gate) { gfx_SetColor(C_GATE); gfx_FillRectangle(xc - w - pwid * 2, y - ph, 2 * w + pwid * 4, bar); }
                else for (k = 0; k < 8; k++) {
                    gfx_SetColor((k & 1) ? C_WHITE : C_BLACK);
                    gfx_FillRectangle(xc - w + k * (w >> 2), y - ph, (w >> 2) + 1, bar);
                }
            } else if ((s & 7) == 0 && s >= 0) {
                int pwid = (w >> 4) + 1, ph = (w * 3) >> 3;
                gfx_SetColor(C_POST);
                gfx_FillRectangle(xc - w - (w >> 1), y - ph, pwid, ph);
                gfx_FillRectangle(xc + w + (w >> 1) - pwid, y - ph, pwid, ph);
            }
            if (n == gn && n < DRAW_SEGS) {
                int gx = px[n] + (px[n + 1] - px[n]) * gf / SEG_LEN;
                int gy = py[n] + (py[n + 1] - py[n]) * gf / SEG_LEN;
                int gw = pw[n] + (pw[n + 1] - pw[n]) * gf / SEG_LEN;
                draw_car(gx + (int)((r->ghost.x * gw) >> 8), gy, gw * 256 / W_REF, 0, GHOST_COL);
            }
        }
        gfx_SetClipRegion(0, 0, GFX_LCD_WIDTH, GFX_LCD_HEIGHT);
    }

    {
        int lean = clampi((c->x - prev_x) >> 3, 5);
        prev_x = c->x;
        draw_car(GFX_LCD_WIDTH / 2, CAR_Y, 256, lean, CAR_COL);
    }

    /* HUD */
    {
        char buf[10], line[16];
        int32_t sp = c->speed < 0 ? -c->speed : c->speed;
        format_time(buf, r->tick);
        hud_text(buf, 4, 4, C_WHITE);
        strcpy(line, "BEST ");
        if (r->best && r->best->ticks) format_time(buf, r->best->ticks); else strcpy(buf, "-:--.--");
        strcat(line, buf);
        hud_text(line, 4, 18, C_WHITE);
        strcpy(u2s(line, (unsigned)((sp >> 8) * 3)), " KM/H");
        hud_text(line, -1, 222, C_WHITE);
        if (r->last_split_cp != 0xFF && r->best && r->best->ticks &&
            (uint16_t)(r->tick - r->run.splits[r->last_split_cp]) < 2 * TICKS_PER_SEC) {
            format_delta(buf, r->last_split_delta);
            hud_text(buf, GFX_LCD_WIDTH / 2 - (int)gfx_GetStringWidth(buf) / 2 - 3, 40,
                     r->last_split_delta < 0 ? C_GREEN : r->last_split_delta > 0 ? C_RED : C_WHITE);
        }
        if (show_fps) { u2s(line, fps); hud_text(line, -1, 4, C_WHITE); }
    }

    if (c->wall_hit) flash = 6;
    if (flash) {
        flash--;
        gfx_SetColor(C_RED);
        gfx_FillRectangle_NoClip(0, 0, GFX_LCD_WIDTH, 4);
        gfx_FillRectangle_NoClip(0, GFX_LCD_HEIGHT - 4, GFX_LCD_WIDTH, 4);
        gfx_FillRectangle_NoClip(0, 4, 4, GFX_LCD_HEIGHT - 8);
        gfx_FillRectangle_NoClip(GFX_LCD_WIDTH - 4, 4, 4, GFX_LCD_HEIGHT - 8);
    }
}

void render_race(const race_t *r, uint8_t show_fps, uint8_t fps)
{
    draw_race(r, show_fps, fps);
    gfx_SwapDraw();
}

static void center_text(const char *s, int y, uint8_t fg) { hud_text(s, GFX_LCD_WIDTH / 2 - 3 - (int)gfx_GetStringWidth(s) / 2, y, fg); }

void render_finish(const race_t *r, uint8_t new_best)
{
    char buf[12], line[20];
    draw_race(r, 0, 0);
    gfx_SetColor(C_HUD);
    gfx_FillRectangle_NoClip(60, 70, 200, 76);
    if (r->finished) {
        format_time(buf, r->run.ticks);
        strcpy(line, "FINISH  ");
        strcat(line, buf);
        center_text(line, 78, C_WHITE);
        if (new_best == 2) center_text("SAVE FAILED", 96, C_RED);   /* new best, but the store failed */
        else if (new_best) center_text("NEW BEST!", 96, C_GREEN);
        else {
            int32_t d = (int32_t)r->run.ticks - r->best->ticks;   /* not new best => a best exists */
            buf[0] = d < 0 ? '-' : '+';
            format_time(buf + 1, (uint16_t)(d < 0 ? -d : d));
            center_text(buf, 96, d < 0 ? C_GREEN : d > 0 ? C_RED : C_WHITE);
        }
    } else center_text("TIME UP", 78, C_RED);
    center_text("2ND RETRY  CLEAR MENU", 122, C_WHITE);
    gfx_SwapDraw();
}

void render_menu(uint8_t sel, const uint16_t *best_ticks)
{
    char buf[10];
    gfx_FillScreen(C_SKY1);
    center_text("TIME TRIAL", 16, C_GATE);
    for (uint8_t i = 0; i < N_TRACKS; i++) {
        int y = 50 + i * 24;
        uint8_t fg = i == sel ? C_GATE : C_WHITE;
        if (i == sel) { gfx_SetColor(C_GATE); gfx_FillRectangle_NoClip(20, y + 1, 6, 10); }
        hud_text(TRACKS[i].name, 32, y, fg);
        if (best_ticks[i]) format_time(buf, best_ticks[i]); else strcpy(buf, "--:--.--");
        hud_text(buf, 220, y, fg);
    }
    center_text("2ND RACE  CLEAR QUIT", 214, C_WHITE);
    gfx_SwapDraw();
}
