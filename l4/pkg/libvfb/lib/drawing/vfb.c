#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include <l4/libvfb/vfb.h>

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))

void vfb_setpixel(l4_int16_t * vscr, int w, int h, int x, int y,
                  l4_int16_t color)
{
    // check bounds
    if (x < 0 || x >= w || y < 0 || y >= h)
        return;
    *(vscr + w * y + x) = color;
}

void vfb_setpixel_rgb(l4_int16_t * vscr, int w, int h, int x, int y,
                      int r, int g, int b)
{
    l4_int16_t color;

    color = vfb_rgb8_to_uint16(r, g, b);
    vfb_setpixel(vscr, w, h, x, y, color);
}

// draws a line from (x, y) to (x, y + h)
void vfb_vbar(l4_int16_t * vscr, int w, int h, int x, int y, int barh,
              l4_int16_t color)
{
    int i;

    if (barh > 0)
        for (i = 0; i <= barh; i++)
            vfb_setpixel(vscr, w, h, x, y + i, color);
    else
        for (i = 0; i >= barh; i--)
            vfb_setpixel(vscr, w, h, x, y + i, color);
}

void vfb_vbar_rgb(l4_int16_t * vscr, int w, int h, int x, int y, int barh,
                  int r, int g, int b)
{
    l4_int16_t color;

    color = vfb_rgb8_to_uint16(r, g, b);
    vfb_vbar(vscr, w, h, x, y, barh, color);
}

void vfb_vbar_rgb_f(l4_int16_t * vscr, int w, int h, int x, int y, int barh,
                    int r, int g, int b)
{
    l4_int16_t color;
    int i, rc, gc, bc;

    if (barh > 0)
        for (i = 0; i <= barh; i++)
        {
            if (barh != 0)
            {
                rc = ((barh - i) * r) / barh;
                gc = ((barh - i) * g) / barh;
                bc = ((barh - i) * b) / barh;
            }
            else
            {
                rc = r;
                gc = g;
                bc = b;
            }
            color = vfb_rgb8_to_uint16(rc, gc, bc);
            vfb_setpixel(vscr, w, h, x, y + i, color);
        }
    else
        for (i = 0; i >= barh; i--)
        {
            if (barh != 0)
            {
                rc = ((barh - i) * r) / barh;
                gc = ((barh - i) * g) / barh;
                bc = ((barh - i) * b) / barh;
            }
            else
            {
                rc = r;
                gc = g;
                bc = b;
            }
            color = vfb_rgb8_to_uint16(rc, gc, bc);
            vfb_setpixel(vscr, w, h, x, y + i, color);
        }
}

// draws a line from (x, y) to (x + w, y)
void vfb_hbar(l4_int16_t * vscr, int w, int h, int x, int y, int barw,
              l4_int16_t color)
{
    int i;

    if (barw > 0)
        for (i = 0; i <= barw; i++)
            vfb_setpixel(vscr, w, h, x + i, y, color);
    else
        for (i = 0; i >= barw; i--)
            vfb_setpixel(vscr, w, h, x + i, y, color);
}

void vfb_hbar_rgb(l4_int16_t * vscr, int w, int h, int x, int y, int barw,
                  int r, int g, int b)
{
    l4_int16_t color;

    color = vfb_rgb8_to_uint16(r, g, b);
    vfb_hbar(vscr, w, h, x, y, barw, color);
}

void vfb_clear_screen(l4_int16_t * vscr, int w, int h)
{
    memset(vscr, 0, w * h * sizeof(vscr[0]));
}

void vfb_fill_rect(l4_int16_t * vscr, int w, int h, int x1, int y1,
                   int x2, int y2, l4_int16_t color)
{
    int x, y;

    for (y = y1; y <= y2; y++)
        for (x = x1; x <= x2; x++)
            vfb_setpixel(vscr, w, h, x, y, color);
}

void vfb_fill_rect_rgb(l4_int16_t * vscr, int w, int h, int x1, int y1,
                       int x2, int y2, int r, int g, int b)
{
    l4_int16_t color;

    color = vfb_rgb8_to_uint16(r, g, b);
    vfb_fill_rect(vscr, w, h, x1, y1, x2, y2, color);
}

/* Draws a grid into a vscreen.
 */
void vfb_draw_grid(l4_int16_t * vscr, int w, int h,
                   int xtics, int ytics, int mxtics, int mytics,
                   int r, int g, int b, int mr, int mg, int mb)
{
#define TIC_SIZE 3
#define MTIC_SIZE 1
    int i, j, x, y;
    l4_int16_t color, mcolor;

    color = vfb_rgb8_to_uint16(r, g, b);
    mcolor = vfb_rgb8_to_uint16(mr, mg, mb);

    // horizontal tics
    for (i = 0; i <= xtics; i++)
    {
        for (j = 1; j < mxtics; j++)
        {
            // mxtics
            x = j * (w / (xtics * mxtics)) + i * (w / xtics);
            vfb_vbar(vscr, w, h, x, 0,                 MTIC_SIZE, mcolor);
            vfb_vbar(vscr, w, h, x, h - 1 - MTIC_SIZE, MTIC_SIZE, mcolor);
        }
        // xtics
        if (i > 0 && i < xtics)
        {
            x = i * (w / xtics);
            vfb_vbar(vscr, w, h, x, 0,                TIC_SIZE, color);
            vfb_vbar(vscr, w, h, x, h - 1 - TIC_SIZE, TIC_SIZE, color);
        }
    }

    // vertical tics
    for (i = 0; i <= ytics; i++)
    {
        for (j = 1; j < mytics; j++)
        {
            y = j * (h / (ytics * mytics)) + i * (h / ytics);
            vfb_hbar(vscr, w, h, 0,                 y, MTIC_SIZE, mcolor);
            vfb_hbar(vscr, w, h, w - 1 - MTIC_SIZE, y, MTIC_SIZE, mcolor);
            // mytics
        }
        // ytics
        if (i > 0 && i < ytics)
        {
            y = i * (h / ytics);
            vfb_hbar(vscr, w, h, 0,                y, TIC_SIZE, color);
            vfb_hbar(vscr, w, h, w - 1 - TIC_SIZE, y, TIC_SIZE, color);
        }
    }
}
