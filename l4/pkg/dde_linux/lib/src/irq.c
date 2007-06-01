/**
 * \file   dde_linux/lib/src/irq.c
 * \brief  IRQ handling and request
 *
 * \date   2007-05-29
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/** \ingroup mod_common
 * \defgroup mod_irq Interrupt Handling
 *
 * This module emulates the interrupt handling inside the Linux kernel.
 *
 * Interrupts are executed each in its own thread that is created using the
 * L4Env Thread library.
 *
 * Requirements: (additionally to \ref pg_req)
 *
 * - Omega0 library and server
 */

/* L4 */
#include <l4/env/errno.h>
#include <l4/omega0/client.h>
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
  struct {
    void (*isr) (int, void *, struct pt_regs *);
                          /**< ISRs */
    unsigned long flags;  /**< flags from request_irq() */
    const char *dev_name; /**< dev_name from request_irq() */
    void *dev_id;         /**< dev_id from request_irq() */
  } h[MAX_IRQ_HANDLERS];
  void (*def_handler) (int, void *);
                        /**< deferred handler */
  void *dev_def_id;     /**< deffered handler id */
} handlers[NR_IRQS];

static int _initialized = 0;  /**< initialization flag */

int l4dde_irq_set_prio(unsigned int irq, unsigned prio)
{
  struct irq_desc *handler = &handlers[irq];

  if ( !(irq < NR_IRQS) || !handler->active )
    return -L4_EINVAL;

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
  irqdesc.s.shared = 1;      /* XXX always try sharing */

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
  omega0_request_t request = { .i=0 };
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
/** IRQ handler thread
 *
 * \param irq_desc  IRQ handling descriptor (IRQ number and handler routine)
 */
static void dde_irq_thread(struct irq_desc *irq_desc)
{
  unsigned int irq = irq_desc->num;     /* save irq description */
  int retval = 0;                       /* thread startup return value */

  int err, irq_handle;
  unsigned int om_flags;

  /* get permission for attaching to IRQ */
  err = __omega0_attach(irq, &irq_handle);
  if (err < 0)
    Panic("failed to attach IRQ %d at omega0!\n", irq);

  if (l4dde_process_add_worker())
    Panic("l4dde_process_add_worker() failed");
  ++local_irq_count(smp_processor_id());

  /* we are up */
  retval = err;
  err = l4thread_started(&retval);

  if ((err < 0) || retval)
    Panic("IRQ thread startup failed!");

  LOGd(DEBUG_MSG, "dde_irq_thread[%d] "l4util_idfmt" running.",
       irq_desc->num, l4util_idstr(l4thread_l4_id(l4thread_myself())));

  om_flags = OM_UNMASK;
  for (;;)
    {
      err = __omega0_wait(irq, irq_handle, om_flags);

      switch (err)
        {
        case 0:
          LOGd(DEBUG_IRQ, "got IRQ %d\n", irq);
          if (irq_desc->active)
            {
              int i;
              for (i = 0; i < MAX_IRQ_HANDLERS; i++)
                if (irq_desc->h[i].isr)
                  irq_desc->h[i].isr(irq, irq_desc->h[i].dev_id, 0);
              if (irq_desc->def_handler)
                irq_desc->def_handler(irq, irq_desc->dev_def_id);
            }
          om_flags = 0;
          break;

        case L4_IPC_RETIMEOUT:
          LOGdL(DEBUG_ERRORS, "Error: timeout while receiving irq");
          break;

        default:
          LOGdL(DEBUG_ERRORS, "Error: receiving irq (%d)", err);
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
 * \todo FIXME consider locking!
 */
int request_irq(unsigned int irq,
                void (*handler) (int, void *, struct pt_regs *),
                unsigned long flags, const char *dev_name, void *dev_id)
{
  l4thread_t irq_tid;
  char name[16];

  if ( !(irq < NR_IRQS) )
    return -EINVAL;
  if (!handler)
    return -EINVAL;

  /* IRQ thread already up ? */
  if (!handlers[irq].t)
    {
      /* create IRQ handler thread */
      LOG_snprintf(name, 16, ".irq%.2X", irq);
      irq_tid = l4thread_create_long(L4THREAD_INVALID_ID,
                                     (l4thread_fn_t) dde_irq_thread,
                                     name,
                                     L4THREAD_INVALID_SP,
                                     L4THREAD_DEFAULT_SIZE,
                                     L4THREAD_DEFAULT_PRIO,
                                     (void *) &handlers[irq],
                                     L4THREAD_CREATE_SYNC);

      if (irq_tid < 0)
        {
          LOGdL(DEBUG_ERRORS, "Error: thread creation failed");
          return -EAGAIN;
        }
      if (*(int*)l4thread_startup_return(irq_tid))
        {
          LOGdL(DEBUG_ERRORS, "Error: irq not free");
          return -EBUSY;
        }
      handlers[irq].t = irq_tid;
    }

  int i;
  for (i = 0; i < MAX_IRQ_HANDLERS; i++)
    if (!handlers[irq].h[i].isr)
      break;

  if (i == MAX_IRQ_HANDLERS)
    {
      LOGd(DEBUG_ERRORS, "Error: maximum number of irq handlers exceeded");
      return -EBUSY;
    }

  handlers[irq].h[i].isr = handler;

  LOGd(DEBUG_IRQ, "new handler for irq %d is %p", irq, handler);

  handlers[irq].h[i].flags = flags;
  handlers[irq].h[i].dev_name = dev_name;
  handlers[irq].h[i].dev_id = dev_id;

  handlers[irq].active = 1;

  LOGd(DEBUG_IRQ, "attached to irq %d", irq);

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
  int i;
  int active = 0;

  for (i = 0; i < MAX_IRQ_HANDLERS; i++)
    {
      if (handlers[irq].h[i].dev_id == dev_id)
        handlers[irq].h[i].isr = NULL;
      if (handlers[irq].h[i].isr)
        active = 1;
    }

  handlers[irq].active = active;

  LOGd(DEBUG_IRQ, "XXX attempt to free irq %d ("l4util_idfmt")", irq,
       l4util_idstr(l4thread_l4_id(handlers[irq].t)));
}

int l4dde_set_deferred_irq_handler(unsigned int irq,
                                   void (*def_handler) (int, void *),
                                   void *dev_def_id)
{
    if (handlers[irq].active==0)
      {
        LOGd(DEBUG_IRQ, "attempt to set deferred handler for free irq %d",
             irq);
        return -L4_EINVAL;
      }
    handlers[irq].dev_def_id = dev_def_id;
    handlers[irq].def_handler = def_handler;
    return 0;
}

/** \name Undocumented
 * \ingroup mod_irq
 * @{ */
void disable_irq(unsigned int irq_num)
{
  LOG("%s not implemented", __FUNCTION__);
}

void disable_irq_nosync(unsigned int irq_num)
{
  LOG("%s not implemented", __FUNCTION__);
}

void enable_irq(unsigned int irq_num)
{
  LOG("%s not implemented", __FUNCTION__);
}

/** Old probing of interrupts
 *\ingroup mod_irq
 *
 * \todo implementation
 */
int probe_irq_on(unsigned long val)
{
  LOG_Error("%s not implemented", __FUNCTION__);
  return 0;
}
/** Old probing of interrupts
 *\ingroup mod_irq
 *
 * \todo implementation
 */
int probe_irq_off(unsigned long val)
{
  LOG_Error("%s not implemented", __FUNCTION__);
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
void synchronize_irq(void)
{
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
int l4dde_irq_init()
{
  int i;

  if (_initialized)
    return -L4_ESKIPPED;

  memset(&handlers, 0, sizeof(handlers));
  for (i = 0; i < NR_IRQS; i++)
    handlers[i].num = i;

  ++_initialized;
  return 0;
}

/** Get IRQ thread number
 * \ingroup mod_irq
 *
 * \param  irq          IRQ number the irq-thread used with request_irq()
 * \return thread-id    threadid of IRQ thread, or L4_INVALID_ID if not
 *                      initialized
 */
l4_threadid_t l4dde_irq_l4_id(int irq)
{
    if (irq >= NR_IRQS) return L4_INVALID_ID;
    if (!handlers[irq].active) return L4_INVALID_ID;
    return l4thread_l4_id(handlers[irq].t);
}

omega0_alien_handler_t l4dde_set_alien_handler(omega0_alien_handler_t handler)
{
    if (!_initialized) return (omega0_alien_handler_t)-1;
    return omega0_set_alien_handler(handler);
}

