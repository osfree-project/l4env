/* $Id$ */
/*
 * Inputlib for Fiasco-UX implementing l4/input/libinput.h
 *
 * by Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 * Some code is taken from l4/pkg/input and l4/pkg/con
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/sys/ipc.h>
#include <l4/sys/vhw.h>
#include <l4/input/libinput.h>
#include <l4/thread/thread.h>
#include <l4/util/macros.h>
#include <l4/util/thread.h>
#include <l4/util/l4_macros.h>
#include <l4/rmgr/librmgr.h>
#include <l4/l4rm/l4rm.h>
#include <l4/env/errno.h>
#include <l4/env/mb_info.h>
#include <l4/sigma0/sigma0.h>
#include <l4/sigma0/kip.h>

#include <stdio.h>

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

static void irq_handler(void *data)
{
  int irq = *(int *)data;
  l4_threadid_t irq_id;
  int error;
  l4_umword_t dw0, dw1;
  l4_msgdope_t result;
  struct l4input ev;

  if (rmgr_get_irq(irq))
    Panic("irq_handler(): "
          "can't get permission for irq 0x%02x, giving up...\n", irq);

  if (l4_is_invalid_id(irq_id = l4util_attach_interrupt(irq)))
    Panic("irq_handler(): "
          "can't attach to irq 0x%02x, giving up...\n", irq);

  if (l4thread_started(NULL))
    Panic("IRQ thread startup failed!\n");

  printf("Started input interrupt thread "l4util_idfmt"!\n",
         l4util_idstr(l4_myself()));

  while (1)
    {
      /* wait for incoming interrupt */
      error = l4_ipc_receive(irq_id, L4_IPC_SHORT_MSG, &dw0, &dw1,
                             L4_IPC_NEVER, &result);

      switch (error)
        {
        case 0:
          /* printf("%s: got IRQ %d\n", __func__, irq); */
          while (dequeue_event(&ev))
            {
              /* printf("%s: irq = %d\n", __func__, irq); */
              input_handler(&ev);
            }
          break;
        default:
          Panic("error receiving irq");
          break;
        }
    }
}

/*
 * Ok, we know that the input memory area is exactly 
 * one page (i.e. 4kB)! If that changes we need to modify this
 * function. But I think it's unlikely that this changes...
 */
static void *map_inputmemory(l4_addr_t paddr)
{
  int error;
  const int size = 1 << 22; // INPUTMEM_SIZE; must be a superpage
  l4_threadid_t my_pager = l4_thread_ex_regs_pager(l4rm_region_mapper_id());
  l4_addr_t vaddr;
  l4_uint32_t rg;

  /* get memory */
  paddr &= L4_PAGEMASK;

  printf("%s: paddr = 0x%08lx\n", __func__, paddr);

  if ((error = l4rm_area_reserve(size, L4RM_LOG2_ALIGNED, &vaddr, &rg)))
    Panic("Error %d reserving region size=%d Bytes for input mem", error, size);

  if ((error = l4sigma0_map_iomem(my_pager, paddr, vaddr, size, 0)))
    {
      switch (error)
	{
	case -1: Panic("Bad alignment of physical address");
	case -2: Panic("IPC error mapping input mem");
	case -3: Panic("No fpage received mapping input mem");
	case -4: Panic("Cannot map input mem at %08lx with old Sigma0 protocol",
		       paddr);
	}
    }

  printf("Input memory page mapped to 0x%08lx\n", vaddr);

  return (void *)vaddr;
}

static int init_stuff(void)
{
  l4thread_t irq_tid;
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

  input_mem = map_inputmemory(vhwe->mem_start);
  irq       = vhwe->irq_no;

  /* only use the interrupt thread if we have a callback function
   * registered; if we're only used in polling mode we already have the
   * data and don't need async notification events
   */
  if (input_handler)
    {
      char buf[7];

      snprintf(buf, sizeof(buf), ".irq%.2X", irq);
      buf[sizeof(buf)-1] = 0;
      irq_tid = l4thread_create_long(L4THREAD_INVALID_ID,
                                     (l4thread_fn_t) irq_handler, buf,
                                     L4THREAD_INVALID_SP,
                                     L4THREAD_DEFAULT_SIZE,
                                     L4THREAD_DEFAULT_PRIO,
                                     (void *)&irq,
                                     L4THREAD_CREATE_SYNC);
      if (irq_tid < 0)
        {
          printf("Error creating IRQ thread!");
          return 1;
        }
    }

  return 0;
}

int l4input_ispending(void)
{
  struct l4input *i = input_mem + input_pos;
  return i->time;
}

int l4input_flush(void *buffer, int count)
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

int l4input_pcspkr(int tone)
{
  return -L4_ENODEV;
}

int l4input_init(int omega0, int prio, void (*handler)(struct l4input *))
{
  input_handler = handler;

  if (init_stuff())
    return 1;

  return 0;
}
