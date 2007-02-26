#ifndef _DROPSCON_COMH_H
#define _DROPSCON_COMH_H

//#include <l4/con/l4con_pslim.h>
//#include <drops/con/con-client.h>
#include <l4/dope/dope-client.h>
#include <l4/dope/pslim-client.h>
#include <asm/semaphore.h>

#define DROPSCON_COMLIST_SIZE	 128

/* originally in l4con.h */
#define L4CON_FAST_COPY		0x00000001
#define L4CON_STREAM_CSCS_YV12	0x00000002
#define L4CON_STREAM_CSCS_YUY2	0x00000004

#define CON_NOVFB		0x00   /*!< dont use vfb */
#define CON_VFB			0x01   /*!< use vfb */

/* ftypes */
#define COMH_NIL    0
#define COMH_FILL   1
#define COMH_PUTS   2
#define COMH_PUTC   3
#define COMH_COPY   4
#define COMH_REDRAW 5

extern int flush_comh_requests;
extern volatile int stop_comh_thread;
extern volatile unsigned int head, tail;
//extern l4_threadid_t dropsvc_l4id;
extern l4_threadid_t pslim_l4id;
extern struct semaphore exit_notify_sem;

typedef struct {
   u16 *p;
   unsigned int x;
   unsigned int y;
   unsigned int columns;
   unsigned int lines;
} comh_redraw_t;

typedef struct {
   pslim_rect_t rect;
   int color;
} comh_fill_t;

typedef struct {
   int x;
   int y;
   /* make sure the whole unit struct fits into 512 bytes */
   unsigned short str[256- 3*2 - sizeof(struct semaphore)/2 - 2];
   int str_size;
} comh_puts_t;

typedef struct {
   int x;
   int y;
   unsigned short ch;
} comh_putc_t;

typedef struct {
   pslim_rect_t rect;
   int dx;
   int dy;
} comh_copy_t;

typedef struct {
   union {
      comh_fill_t fill;
      comh_puts_t puts;
      comh_putc_t putc;
      comh_redraw_t redraw;
      comh_copy_t copy;
   } func;
   int ftype;
#ifdef COMH_KERNEL_THREAD
   struct semaphore sem;
#else
   atomic_t valid;
#endif
} comh_proto_t;

extern comh_proto_t comh_list[DROPSCON_COMLIST_SIZE];

void comh_thread(void*);

#ifndef COMH_KERNEL_THREAD
extern atomic_t comh_sleep_state;
extern void comh_wakeup(void);
#endif

#endif
