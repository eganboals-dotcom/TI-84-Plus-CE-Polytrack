/* Host mirror of the CE graphx subset used by render.c / main.c. 320x240 8-bit double buffer + 1555 palette. */
#ifndef SHIM_GRAPHX_H
#define SHIM_GRAPHX_H
#include <stdint.h>
typedef uint32_t uint24_t;
typedef enum { gfx_screen = 0, gfx_buffer } gfx_location_t;
#define GFX_LCD_WIDTH 320
#define GFX_LCD_HEIGHT 240
#define gfx_RGBTo1555(r,g,b) \
((uint16_t)(((((r) & 255) >> 3) << 10) | ((((g) & 255) >> 3) << 5) | (((b) & 255) >> 3)))
#define gfx_SetDrawBuffer() gfx_SetDraw(gfx_buffer)
#define gfx_SetDrawScreen() gfx_SetDraw(gfx_screen)

void gfx_Begin(void);
void gfx_End(void);
void gfx_SetDraw(uint8_t location);
void gfx_SwapDraw(void);
void gfx_Wait(void);
uint8_t (*shim_vbuffer(void))[240][320];
#define gfx_vbuffer (*shim_vbuffer())   /* the draw buffer, like the CE macro */
void gfx_SetPalette(const void *palette, uint24_t size, uint8_t offset);
uint8_t gfx_SetColor(uint8_t index);
void gfx_FillScreen(uint8_t index);
void gfx_HorizLine_NoClip(uint24_t x, uint8_t y, uint24_t length);
void gfx_FillRectangle(int x, int y, int width, int height);
void gfx_FillRectangle_NoClip(uint24_t x, uint8_t y, uint24_t width, uint8_t height);
void gfx_FillTriangle(int x0, int y0, int x1, int y1, int x2, int y2);
void gfx_SetClipRegion(int xmin, int ymin, int xmax, int ymax);
void gfx_PrintStringXY(const char *string, int x, int y);
uint8_t gfx_SetTextFGColor(uint8_t color);
uint8_t gfx_SetTextBGColor(uint8_t color);
uint8_t gfx_SetTextTransparentColor(uint8_t color);
unsigned int gfx_GetStringWidth(const char *string);

int shim_dump_ppm(const char *path);   /* visible screen -> binary PPM; 0 on failure */
#endif
