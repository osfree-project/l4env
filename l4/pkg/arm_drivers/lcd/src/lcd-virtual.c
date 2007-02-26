/*
 * Dummy LCD driver
 */

#include <l4/arm_drivers/lcd.h>
#include <stdlib.h>

static const char *arm_lcd_none_get_info(void)
{ return "ARM LCD virtual driver"; }

static void void_op_uint_uint_uint_uint
  (struct arm_lcd_ops *o, unsigned int x, unsigned int y,
                          unsigned int w, unsigned h) {}
static void void_op_uint(struct arm_lcd_ops *o, unsigned int val) {}
static void void_dummy(void) {}

static int probe(void)             { return 0; }
static unsigned int width(void)    { return 320; }
static unsigned int height(void)   { return 200; }
static unsigned int bpp(void)      { return 32;  }
static unsigned int bpl(void)      { return (32 >> 3) * width(); }
static unsigned int mem_size(void) { return height() * bpl(); }
static void *fb(void)              { return malloc(mem_size()); }

static struct arm_lcd_ops arm_lcd_ops_virtual = {
  .probe              = probe,
  .get_fb             = fb,
  .get_video_mem_size = mem_size,
  .get_screen_width   = width,
  .get_screen_height  = height,
  .get_bpp            = bpp,
  .get_bytes_per_line = bpl,
  .get_info           = arm_lcd_none_get_info,
  .enable             = void_dummy,
  .disable            = void_dummy,
  .area_set           = void_op_uint_uint_uint_uint,
  .area_put_pixel16   = void_op_uint,
};

arm_lcd_register(&arm_lcd_ops_virtual);
