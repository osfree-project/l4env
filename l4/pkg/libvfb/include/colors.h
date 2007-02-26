#ifndef __LIBVFB_INCLUDE_COLORS_H_
#define __LIBVFB_INCLUDE_COLORS_H_

#include <l4/sys/types.h>
#include <l4/libvfb/vfb.h> // for vfb_rgb8_to_uint16

/* taken from Dope */
/*** BLEND 16BIT COLOR WITH SPECIFIED ALPHA VALUE ***/
static inline l4_uint16_t vfb_blend_color(l4_uint16_t color, int alpha) {
	return ((((alpha >> 3) * (color & 0xf81f)) >> 5) & 0xf81f)
	      | (((alpha * (color & 0x07e0)) >> 8) & 0x7e0);
}

/* some helper to generate lighter or darker colors */
static inline l4_uint16_t vfb_lighten_color(l4_uint16_t color)
{
    return vfb_blend_color(color, 128) + vfb_blend_color(255, 128);
}

static inline l4_uint16_t vfb_darken_color(l4_uint16_t color)
{
    return vfb_blend_color(color, 128);
}

/* some well defined colors */
#define rgb16_black       0
#define rgb16_red         vfb_rgb8_to_uint16(255, 0, 0)
#define rgb16_green       vfb_rgb8_to_uint16(0, 255, 0)
#define rgb16_yellow      vfb_rgb8_to_uint16(255, 255, 0)
#define rgb16_blue        vfb_rgb8_to_uint16(0, 0, 255)
#define rgb16_magenta     vfb_rgb8_to_uint16(255, 0, 255)
#define rgb16_cyan        vfb_rgb8_to_uint16(0, 255, 255)
#define rgb16_white       vfb_rgb8_to_uint16(255, 255, 255)

#endif
