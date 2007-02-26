/*!
 * \file	con/examples/linux_stub/comh.h
 * \brief	
 *
 * \date	01/2002
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __CON_EXAMPLES_LINUX_STUB_COMH_H_
#define __CON_EXAMPLES_LINUX_STUB_COMH_H_

#include <l4/l4con/l4con_pslim.h>
#include <l4/l4con/l4con-client.h>
#include <asm/semaphore.h>

#define DROPSCON_COMLIST_SIZE	 128

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
extern l4_threadid_t dropsvc_l4id;
extern struct semaphore exit_notify_sem;

typedef struct {
   u16 *p;
   unsigned int x;
   unsigned int y;
   unsigned int columns;
   unsigned int lines;
} comh_redraw_t;

typedef struct {
   l4con_pslim_rect_t rect;
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
   l4con_pslim_rect_t rect;
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
   atomic_t valid;
} comh_proto_t;

extern comh_proto_t comh_list[DROPSCON_COMLIST_SIZE];

void comh_thread(void*);

extern l4_uint32_t comh_sleep_state;
extern void comh_wakeup(void);

#endif
