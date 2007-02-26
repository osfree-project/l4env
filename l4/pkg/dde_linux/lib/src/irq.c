/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/lib/src/irq.c
 * \brief  IRQ handling and request
 *
 * \date   08/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/** \ingroup mod_common
 * \defgroup mod_irq Interrupt Handling
 *
 * This module emulates the interrupt handling inside the Linux kernel. It is
 * derived from several existing DROPS emulation modules.
 *
 * There are 2 different approaches to manage interrupts under DROPS/L4: the
 * root resource manager RMGR or Omega0 (resp. the L4Env I/O Server). The
 * Interrupt Handling Module has to be initialized via #irq_init() specifying
 * the environment you are using as parameter.
 *
 * Interrupts are executed each in its own thread that is created using the
 * L4Env Thread library.
 *
 * Requirements: (additionally to \ref pg_req)
 *
 * - Omega0 library
 * - optional: Omega0 server if configured at runtime
 * - initialized RMGR library
 *
 * Configuration:
 *
 * - configure use of Omega0 resp. RMGR via l4dde_irq_init()
 */
/*****************************************************************************/

/* L4 */
#include <l4/env/errno.h>
#include <l4/omega0/client.h>
#include <l4/rmgr/librmgr.h>
#include <l4/util/irq.h>
#include <l4/util/thread.h>  /* attach_interrupt() */
#include <l4/thread/thread.h>
#include <l4/lock/lock.h>

#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/sched.h>
#include <asm/hardirq.h>
#include <asm/irq.h>

/* local */
#include "__config.h"
#include "internal.h"

/** \name Module Variables
 * @{ */

/** IRQ descriptor array.
 * \todo what about IRQ sharing? */
static struct irq_desc
{
  int active;           /**< IRQ in use */
  int num;              /**< IRQ number */
  l4thread_t t;         /**< handler thread */
  void (*handler) (int, void *, struct pt_regs *);
                        /**< ISR */
  unsigned long flags;  /**< flags from request_irq() */
  const char *dev_name; /**< dev_name from request_irq() */
  void *dev_id;         /**< dev_id from request_irq() */
} handlers[NR_IRQS];

/** Usage flag.
 * If 1 use Omega0 and if 0 use RMGR for interrupts. */
static int use_omega0 = 0;

static int _initialized = 0;  /**< initialization flag */

int l4dde_irq_set_prio(unsigned int irq, unsigned prio){
    struct irq_desc *handler;

    /* Oh, I remember, currently only one handler per IRQ is allowed. */
    handler = &handlers[irq];
    if(!handler) return -L4_EINVAL;
    return l4thread_set_prio(handler->t, prio);
}


/** @} */
/** \name Omega0 specific routines
 * @{ */

#define OM_MASK    0x00000001
#define OM_UNMASK  0x00000002
#define OM_CONSUME 0x00000004

/** Attach IRQ line.
 *
 * \param  irq     IRQ number
 * \retval handle  IRQ handle
 *
 * \return 0 on success (attached interrupt), -1 if attach failed.
 */
static inline int __omega0_attach(unsigned int irq, int *handle)
{
  omega0_irqdesc_t irqdesc;

  /* setup irq descriptor */
  irqdesc.s.num = irq + 1;
  irqdesc.s.shared = 0;

  /* attach IRQ */
  *handle = omega0_attach(irqdesc);
  if (*handle < 0)
    return -1;
  else
    return 0;
}

/** Wait for IRQ notification.
 *
 * \param  irq     IRQ number
 * \param  handle  IRQ line handle
 * \param  flags   Flags:
 *                 - \c OM_MASK    request IRQ mask
 *                 - \c OM_UNMASK  request IRQ unmask
 *                 - \c OM_CONSUME IRQ consumed
 */
static inline int __omega0_wait(unsigned int irq, int handle, unsigned int flags)
{
  omega0_request_t request;
  int ret;

  /* setup omega0 request */
  request.s.param = irq + 1;
  request.s.wait = 1;
  request.s.consume = (flags & OM_CONSUME) ? 1 : 0;
  request.s.mask = (flags & OM_MASK) ? 1 : 0;
  request.s.unmask = (flags & OM_UNMASK) ? 1 : 0;

  /* wait for interrupt */
  ret = omega0_request(handle, request);
  if (ret != (irq + 1))
    Panic("error waiting for interrupt\n");

  return 0;
}

/** @} */
/** \name RMGR specific routines and PIC handling
 * @{ */

/** Enable IRQ
 *
 * \param  irq  IRQ number
 */
static inline void __enable_irq(unsigned int irq)
{
  int port;
  l4util_cli();
  port = (((irq & 0x08) << 4) + 0x21);
  l4util_out8(l4util_in8(port) & ~(1 << (irq & 7)), port);
  l4util_sti();
}

/** Disable IRQ
 *
 * \param  irq  IRQ number
 */
static inline void __disable_irq(unsigned int irq)
{
  unsigned short port;
  l4util_cli();
  port = (((irq & 0x08) << 4) + 0x21);
  l4util_out8(l4util_in8(port) | (1 << (irq & 7)), port);
  l4util_sti();
}

/** Disable and acknowledge IRQ
 *
 * \param  irq  IRQ number
 */
static inline void __ack_irq(unsigned int irq)
{
  l4util_irq_acknowledge(irq);
}

/** @} */
/** IRQ handler thread
 *
 * \param irq_desc  IRQ handling descriptor (IRQ number and handler routine)
 */
static void dde_irq_thread(struct irq_desc *irq_desc)
{
  unsigned int irq = irq_desc->num;     /* save irq description */
  int retval = 0;                       /* thread startup return value */

  int ret, error = L4_IPC_RETIMEOUT, irq_handle;
  l4_threadid_t irq_id;
  l4_umword_t dw0, dw1;
  l4_msgdope_t result;
  unsigned int om_flags;

  /* get permission for attaching to IRQ */
  if (use_omega0)
    {
      ret = __omega0_attach(irq, &irq_handle);
      if (ret < 0)
        Panic("failed to attach IRQ %d at omega0!\n", irq);
      else
        error = L4_IPC_RETIMEOUT;
    }
  else
    {
      if (rmgr_get_irq(irq))
        /* can't get permission -> block */
        Panic("%s: can't get permission for irq 0x%02x, giving up...\n",
              __FUNCTION__, irq);

      /* attach to IRQ */
      if (l4_is_invalid_id(irq_id = l4util_attach_interrupt(irq)))
        Panic("%s: can't attach to irq 0x%02x, giving up...\n",
              __FUNCTION__, irq);

      /* read spurious interrupts */
      for (;;)
        {
          error = l4_ipc_receive(irq_id,
                                      L4_IPC_SHORT_MSG, &dw0, &dw1,
                                      L4_IPC_TIMEOUT(0, 0, 1, 15, 0, 0), &result);
          if (error == L4_IPC_RETIMEOUT)
            break;
        }
    }

  ++local_irq_count(smp_processor_id());

  /* we are up */
  retval = (error != L4_IPC_RETIMEOUT) ? 1 : 0;
  ret = l4thread_started(&retval);

  if ((ret < 0)|| retval)
    Panic("IRQ thread startup failed!");

  DMSG("dde_irq_thread[%d] "IdFmt" running.\n",
       irq_desc->num, IdStr(l4thread_l4_id(l4thread_myself())));

  if (!use_omega0)
    __enable_irq(irq);
  om_flags = OM_UNMASK;
  for (;;)
    {
      if (use_omega0)
        error = __omega0_wait(irq, irq_handle, om_flags);
      else
        {
          /* wait for incoming interrupt */
          error = l4_ipc_receive(irq_id,
                                      L4_IPC_SHORT_MSG, &dw0, &dw1,
                                      L4_IPC_NEVER, &result);
          __disable_irq(irq);
          __ack_irq(irq);
        }

      switch (error)
        {
        case 0:
#if DEBUG_IRQ
        DMSG("got IRQ %d\n", irq);
#endif
          if (irq_desc->active)
            irq_desc->handler(irq, irq_desc->dev_id, 0);
          if (use_omega0)
            om_flags = 0;
          else
            __enable_irq(irq);
          break;
        case L4_IPC_RETIMEOUT:
          ERROR("timeout while receiving irq");
          break;
        default:
          ERROR("receiving irq (%d)", error);
          break;
        }
    }
}

/** Request Interrupt.
 * \ingroup mod_irq
 *
 * \param  irq       interrupt number
 * \param  handler   interrupt handler -> top half
 * \param  flags     interrupt handling flags (SA_SHIRQ, ...)
 * \param  dev_name  device name
 * \param  dev_id    cookie passed back to handler
 *
 * \return 0 on success; error code otherwise
 *
 * \todo reattachment
 */
int request_irq(unsigned int irq,
                void (*handler) (int, void *, struct pt_regs *),
                unsigned long flags, const char *dev_name, void *dev_id)
{
  l4thread_t irq_tid;

  if (irq >= NR_IRQS)
    return -EINVAL;
  if (!handler)
    return -EINVAL;

  if(handlers[irq].t)
    {
      if (handlers[irq].active)
        {
#if DEBUG_IRQ
          DMSG("XXX attempt to reattach to active irq %d\n", irq);
#endif
          return -EBUSY;
        }
      handlers[irq].active = 1;

      /* XXX what about handler, flags, ... ? */

#if DEBUG_IRQ
      DMSG("reattached to irq %d\n", irq);
#endif
      return 0;
    }

  handlers[irq].handler = handler;

  /* create IRQ handler thread */
  irq_tid = l4thread_create((l4thread_fn_t) dde_irq_thread,
                            (void *) &handlers[irq], L4THREAD_CREATE_SYNC);

  if (irq_tid<0)
    {
      ERROR("thread creation failed");
      return -EAGAIN;
    }
  if (*(int*)l4thread_startup_return(irq_tid))
    {
      ERROR("irq not free");
      return -EBUSY;
    }

  handlers[irq].active = 1;
  handlers[irq].t = irq_tid;

  handlers[irq].flags = flags;
  handlers[irq].dev_name = dev_name;
  handlers[irq].dev_id = dev_id;

#if DEBUG_IRQ
  DMSG("attached to irq %d\n", irq);
#endif

  return 0;
}

/** Release Interrupt
 * \ingroup mod_irq
 *
 * \param  irq     interrupt number
 * \param  dev_id  cookie passed back to handler
 *
 * \todo Stop IRQ-Thread
 *
 * \todo Release IRQ
 */
void free_irq(unsigned int irq, void *dev_id)
{
  handlers[irq].active = 0;

#if DEBUG_IRQ
  DMSG("XXX attempt to free irq %d ("IdFmt")\n", irq,
       IdStr(l4thread_l4_id(handlers[irq].t)));
#endif
}


/** \name Undocumented
 * \ingroup mod_irq
 * @{ */
void disable_irq(unsigned int irq_num)
{
  Msg("%s not implemented", __FUNCTION__);
}

void disable_irq_nosync(unsigned int irq_num)
{
  Msg("%s not implemented", __FUNCTION__);
}

void enable_irq(unsigned int irq_num)
{
  Msg("%s not implemented", __FUNCTION__);
}

/** Old probing of interrupts
 *\ingroup mod_irq
 *
 * \todo implementation
 */
int probe_irq_on(unsigned long val)
{
  Error("%s not implemented", __FUNCTION__);
  return 0;
}
/** Old probing of interrupts
 *\ingroup mod_irq
 *
 * \todo implementation
 */
int probe_irq_off(unsigned long val)
{
  Error("%s not implemented", __FUNCTION__);
  return 0;
}

/*
 * This is called when we want to synchronize with
 * interrupts. We may for example tell a device to
 * stop sending interrupts: but to make sure there
 * are no interrupts that are executing on another
 * CPU we need to call this function.
 *
 * However, in non-SMP context, it is a define to barrier()
 */
#ifdef CONFIG_SMP
void synchronize_irq(void){
#warning Not implemented.

  /* To implement this, I think it is better not to do a cli()/sti() as Linux
     does, but to wait until each of the current active interrupts was
     outside its interrupt loop once. Otherwise, the interrupt-code is blocked
     unnecessarily, which should be avoided.
  */
}
#endif

/** @} */
/** Initalize IRQ handling
 * \ingroup mod_irq
 *
 * \param omega0  If set use Omega0 interrupt handling - if unset use RMGR
 *                directly
 *
 * \return 0 on success; negative error code otherwise
 */
int l4dde_irq_init(int omega0)
{
  int i;

  if (_initialized)
    return -L4_ESKIPPED;

  use_omega0 = omega0;

  memset(&handlers, 0, sizeof(handlers));
  for (i=0; i<NR_IRQS;i++)
    handlers[i].num = i;

  ++_initialized;
  return 0;
}
