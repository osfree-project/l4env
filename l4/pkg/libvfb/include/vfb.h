#ifndef __LIBVFB_INCLUDE_VFB_H_
#define __LIBVFB_INCLUDE_VFB_H_

#include <l4/sys/types.h>

#define RGB565_R_MASK 0xF8
#define RGB565_G_MASK 0xFC
#define RGB565_B_MASK 0xF8

#define RGB565_R_L_SHIFT 8
#define RGB565_G_L_SHIFT 3
#define RGB565_B_R_SHIFT 3

#define vfb_rgb8_to_uint16(r, g, b)               \
    (((RGB565_R_MASK & r) << RGB565_R_L_SHIFT) |  \
     ((RGB565_G_MASK & g) << RGB565_G_L_SHIFT) |  \
     ((RGB565_B_MASK & b) >> RGB565_B_R_SHIFT))

void vfb_setpixel(l4_int16_t * vscr, int w, int h, int x, int y,
                  l4_int16_t color);
void vfb_setpixel_rgb(l4_int16_t * vscr, int w, int h, int x, int y,
                  int r, int g, int b);

void vfb_vbar(l4_int16_t * vscr, int w, int h, int x, int y, int barh,
              l4_int16_t color);
void vfb_vbar_rgb(l4_int16_t * vscr, int w, int h, int x, int y, int barh,
                  int r, int g, int b);
void vfb_vbar_rgb_f(l4_int16_t * vscr, int w, int h, int x, int y, int barh,
                    int r, int g, int b);

void vfb_hbar(l4_int16_t * vscr, int w, int h, int x, int y, int barw,
              l4_int16_t color);
void vfb_hbar_rgb(l4_int16_t * vscr, int w, int h, int x, int y, int barw,
                  int r, int g, int b);

void vfb_fill_rect(l4_int16_t * vscr, int w, int h, int x1, int y1,
                   int x2, int y2, l4_int16_t color);
void vfb_fill_rect_rgb(l4_int16_t * vscr, int w, int h, int x1, int y1,
                       int x2, int y2, int r, int g, int b);

void vfb_clear_screen(l4_int16_t * vscr, int w, int h);

void vfb_draw_grid(l4_int16_t * vscr, int w, int h,
                   int xtics, int ytics, int mxtics, int mytics,
                   int r, int g, int b, int mr, int mg, int mb);

#endif
