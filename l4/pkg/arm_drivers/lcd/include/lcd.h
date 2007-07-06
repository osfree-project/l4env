#ifndef __ARM_DRIVERS__LCD__INCLUDE__LCD_H__
#define __ARM_DRIVERS__LCD__INCLUDE__LCD_H__

#include <l4/sys/types.h>
#include <l4/crtx/ctor.h>

EXTERN_C_BEGIN

struct arm_lcd_ops {
  int          (*probe)(void);
  void *       (*get_fb)(void);
  unsigned int (*get_video_mem_size)(void);
  unsigned int (*get_screen_width)(void);
  unsigned int (*get_screen_height)(void);
  unsigned int (*get_bpp)(void);
  unsigned int (*get_bytes_per_line)(void);
  const char * (*get_info)(void);

  void         (*enable)(void);
  void         (*disable)(void);

  void         (*area_set)(struct arm_lcd_ops *s,
                           unsigned int x, unsigned int y,
                           unsigned int w, unsigned int h);
  void         (*area_put_pixel16)(struct arm_lcd_ops *s,
                                   unsigned int val16);

  /* Some values */
  unsigned int area_x, area_y, area_w, area_h;
};

struct arm_lcd_ops *arm_lcd_probe(void);

int arm_lcd_get_region(l4_addr_t addr, l4_size_t size);

void arm_lcd_register_driver(struct arm_lcd_ops *);

/* Callable once per file (should be enough?) */
#define arm_lcd_register(ops)                                           \
    static void __register_ops(void) { arm_lcd_register_driver(ops); }  \
    L4C_CTOR(__register_ops, L4CTOR_AFTER_BACKEND)

EXTERN_C_END

#endif /* ! __ARM_DRIVERS__LCD__INCLUDE__LCD_H__ */
