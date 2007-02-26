#ifndef _INT10_H
#define _INT10_H

#include <l4/util/mb_info.h>

int
int10_set_vbemode(int mode, l4util_mb_vbe_ctrl_t **ctrl_info,
		  l4util_mb_vbe_mode_t **mode_info);

#endif
