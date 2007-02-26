#include <l4/x86emu/int10.h>

int
x86emu_int10_set_vbemode(int mode, l4util_mb_vbe_ctrl_t **ctrl_info,
			 l4util_mb_vbe_mode_t **mode_info)
{
  return -1;
}

int
x86emu_int10_pan(unsigned *x, unsigned *y)
{
  return -1;
}

int
x86emu_int10_done(void)
{
  return 0;
}
