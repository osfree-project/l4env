/* $Id$ */
/*
 * Inputlib for Fiasco-UX implementing l4/input/libinput.h
 *
 * by Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 * Some code is taken from l4/pkg/input and l4/pkg/con
 *
 * Adaptions by Christian Helmuth <ch12@os.inf.tu-dresden.de>
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/sys/vhw.h>
#include <l4/input/libinput.h>
#include <l4/util/macros.h>
#include <l4/env/errno.h>
#include <l4/sigma0/kip.h>
#include <l4/generic_io/libio.h>

#include <linux/interrupt.h>

#include <stdio.h>

#include "internal.h"

/* needs to be the same as in ux_con! */
enum {
  INPUTMEM_SIZE = 1 << 12,
  NR_INPUT_OBJS = INPUTMEM_SIZE / sizeof(struct l4input),
};

static int input_pos;
static struct l4input *input_mem;
static void (*input_handler)(struct l4input *);

static int dequeue_event(struct l4input *ev)
{
  struct l4input *i = input_mem + input_pos;

  if (i->time == 0)
    return 0;
  i->time = 0;

  *ev = *i;

  input_pos++;
  if (input_pos == NR_INPUT_OBJS)
    input_pos = 0;

  return 1;
}

static irqreturn_t irq_handler(int irq,
                               void *dev_id, struct pt_regs *regs)
{
  struct l4input ev;

  /* printf("%s: got IRQ %d\n", __func__, irq); */
  while (dequeue_event(&ev))
    {
      /* printf("%s: irq = %d\n", __func__, irq); */
      input_handler(&ev);
    }

  return 0;
}

static void *map_inputmemory(l4_addr_t paddr, l4_size_t size)
{
  l4_addr_t vaddr = l4io_request_mem_region(paddr, size, L4IO_MEM_CACHED);
  if (!vaddr)
    Panic("Mapping input memory from %p failed", (void *)paddr);

  printf("Input memory page mapped to 0x%08lx\n", vaddr);

  return (void *)vaddr;
}

static int init_stuff(void)
{
  int irq;
  l4_kernel_info_t *kip = l4sigma0_kip_map(L4_INVALID_ID);
  struct l4_vhw_descriptor *vhw;
  struct l4_vhw_entry *vhwe;

  if (!kip)
    Panic("Cannot map KIP!");

  if (!(vhw = l4_vhw_get(kip))
      || !(vhwe = l4_vhw_get_entry_type(vhw, L4_TYPE_VHW_INPUT))
      || !vhwe->irq_no)
    Panic("Cannot read VHW structure!");

  input_mem = map_inputmemory(vhwe->mem_start, vhwe->mem_size);
  irq       = vhwe->irq_no;

  /* only use the interrupt thread if we have a callback function
   * registered; if we're only used in polling mode we already have the
   * data and don't need async notification events
   */
  if (input_handler)
    {
      int err = request_irq(irq, irq_handler, 0, "", 0);
      if (err)
        {
          printf("Error creating IRQ thread!");
          return 1;
        }
    }

  return 0;
}

static int ux_ispending(void)
{
  struct l4input *i = input_mem + input_pos;
  return i->time;
}

static int ux_flush(void *buffer, int count)
{
  int c = 0;
  struct l4input *b = buffer;
  struct l4input *i = input_mem + input_pos;

  while (count && i->time)
    {
      *b = *i;

      i->time = 0;

      b++;

      input_pos++;
      if (input_pos == NR_INPUT_OBJS)
        input_pos = 0;
      i = input_mem + input_pos;

      count--;
      c++;
    }

  return c;
}

static struct l4input_ops ops = { ux_ispending, ux_flush, 0 };

struct l4input_ops * l4input_internal_ux_init(void (*handler)(struct l4input *))
{
  input_handler = handler;

  if (init_stuff())
    return 0;

  return &ops;
}
