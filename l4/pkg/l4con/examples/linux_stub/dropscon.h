/*!
 * \file	con/examples/linux_stub/dropscon.h
 * \brief	
 *
 * \date	01/2002
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

#ifndef __CON_EXAMPLES_LINUX_STUB_DROPSCON_H_
#define __CON_EXAMPLES_LINUX_STUB_DROPSCON_H_

#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
#define L4L22
#else
#define L4L24
#endif

/* receive buffer size at con server side */
#define DROPSCON_MAX_SBUF_SIZE	8192
#define DROPSCON_FIRST_VC	0
#define DROPSCON_LAST_VC	5

/* XXX font size should be asked from console server */
extern unsigned int fn_x, fn_y;
#define DROPSCON_BITX(x)	((x) * fn_x)
#define DROPSCON_BITY(y)	((y) * fn_y)

extern l4_threadid_t main_l4id;
extern l4_threadid_t ev_l4id;
extern l4_threadid_t con_l4id;
extern l4_threadid_t dropsvc_l4id;

extern unsigned int fg_color, bg_color;
extern unsigned int dropscon_num_lines, dropscon_num_columns;
extern unsigned int accel_flags;
extern unsigned int init_done;
extern unsigned int redraw_pending;
extern struct vc_data *dropscon_display_fg;

void dropscon_redraw_all(void);
void dropscon_clear_gap(void);

int  con_kbd_init(void);
void con_kbd_exit(void);

#endif

