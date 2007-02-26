/*
 *  dropscon.h -- DROPS textbased console driver
 */

#ifndef _DROPSCON_H
#define _DROPSCON_H

#include <linux/version.h>

#if LINUX_VERSION_CODE < KERNEL_VERSION(2,4,0)
#define L4L22
#else
#define L4L24
#endif

//#define OVL

/* receive buffer size at con server side */
#define DROPSCON_MAX_SBUF_SIZE	8192
#define DROPSCON_FIRST_VC	0
#define DROPSCON_LAST_VC	5

#define STUBMODE_CON 	1
#define STUBMODE_DOPE 	2
extern int stubmode;

/* XXX font size should be asked from console server */
extern unsigned int fn_x, fn_y;
#define DROPSCON_BITX(x)	((x) * fn_x)
#define DROPSCON_BITY(y)	((y) * fn_y)

//#define dropsvc_l4id pslim_l4id
extern l4_threadid_t ev_l4id;
extern l4_threadid_t con_l4id;
extern l4_threadid_t pslim_l4id;
extern l4_threadid_t main_l4id;
#ifdef OVL
extern l4_threadid_t ovl_l4id;
#endif
//extern l4_threadid_t dropsvc_l4id;

extern unsigned int fg_color, bg_color;
extern unsigned int dropscon_num_lines, dropscon_num_columns;
extern unsigned int accel_flags;
extern unsigned int init_done;
extern unsigned int redraw_pending;
extern unsigned int xres, yres, depth;
extern struct vc_data *dropscon_display_fg;

void dropscon_redraw_all(void);
void dropscon_clear_gap(void);

int  dope_kbd_init(void);
void dope_kbd_exit(void);

int  con_kbd_init(void);
void con_kbd_exit(void);

#ifdef OVL
int  ovl_kbd_init(void);
#endif

#ifdef L4L22
#include <asm/l4_debug.h>
#define printf	herc_printf
#endif

#endif

