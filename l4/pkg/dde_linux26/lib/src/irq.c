/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux26/lib/src/irq.c
 *
 * \brief	IRQ handling and request
 *
 * \author	Marek Menzer <mm19@os.inf.tu-dresden.de>
 *
 * Based on dde_linux version by Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * and Linux 2.6
 */
/*****************************************************************************/
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
#include <l4/util/thread.h>	/* attach_interrupt() */
#include <l4/thread/thread.h>
#include <l4/lock/lock.h>

#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/sched.h>
#include <asm/irq.h>
#include <linux/errno.h>
#include <linux/interrupt.h>

/* local */
#include "__config.h"
#include "__macros.h"
#include "internal.h"

/*****************************************************************************/
/** 
 * \name Module Variables
 * @{ */
/*****************************************************************************/
/** IRQ descriptor array.
 * \todo what about IRQ sharing? */
static struct irq_desc handlers[NR_IRQS];

/** Usage flag.
 * If 1 use Omega0 and if 0 use RMGR for interrupts. */
static int use_omega0 = 0;

static int _initialized = 0;	/**< initialization flag */

static void end_irq(unsigned int irq);

int l4dde_irq_set_prio(unsigned int irq, unsigned prio){
    struct irq_desc *handler;
    
    handler = &handlers[irq];
    if (!handler) return -L4_EINVAL;
    return l4thread_set_prio(handler->t, prio);
}

/** @} */
/*****************************************************************************/
/**
 * \name Omega0 specific routines
 *
 * @{ */
/*****************************************************************************/
#define OM_MASK    0x00000001
#define OM_UNMASK  0x00000002
#define OM_CONSUME 0x00000004

/*****************************************************************************/
/** Attach IRQ line.
 * 
 * \param  irq           IRQ number
 * \retval handle        IRQ handle
 *	
 * \return 0 on success (attached interrupt), -1 if attach failed.
 */
/*****************************************************************************/
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

/*****************************************************************************/
/** Detach IRQ line.
 * 
 * \param  irq           IRQ number
 *	
 * \return 0 on success (detached interrupt), -1 if detach failed.
 */
/*****************************************************************************/
static inline int __omega0_detach(unsigned int irq)
{
  omega0_irqdesc_t irqdesc;

  /* setup irq descriptor */
  irqdesc.s.num = irq + 1;
  irqdesc.s.shared = 0;

  /* attach IRQ */
  if (omega0_detach(irqdesc) < 0)
    return -1;
  else
    return 0;
}

/*****************************************************************************/
/** Wait for IRQ notification.
 * 
 * \param  irq           IRQ number
 * \param  handle        IRQ line handle
 * \param  flags         Flags:
 *                       - \c OM_MASK    request IRQ mask
 *                       - \c OM_UNMASK  request IRQ unmask
 *                       - \c OM_CONSUME IRQ consumed
 */
/*****************************************************************************/
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
/*****************************************************************************/
/**
 * \name RMGR specific routines and PIC handling
 *
 * @{ */
/*****************************************************************************/
/*****************************************************************************/
/** Enable IRQ
 * 
 * \param  irq           IRQ number
 */
/*****************************************************************************/
static inline void __enable_irq(unsigned int irq)
{
  int port;
  l4util_cli();
  port = (((irq & 0x08) << 4) + 0x21);
  l4util_out8(l4util_in8(port) & ~(1 << (irq & 7)), port);
  l4util_sti();
}

/*****************************************************************************/
/** Disable IRQ
 * 
 * \param  irq           IRQ number
 */
/*****************************************************************************/
static inline void __disable_irq(unsigned int irq)
{
  unsigned short port;
  l4util_cli();
  port = (((irq & 0x08) << 4) + 0x21);
  l4util_out8(l4util_in8(port) | (1 << (irq & 7)), port);
  l4util_sti();
}

/*****************************************************************************/
/** Disable and acknowledge IRQ
 * 
 * \param  irq           IRQ number
 */
/*****************************************************************************/
static inline void __ack_irq(unsigned int irq)
{
  l4util_irq_acknowledge(irq);
}

/** @} */
/*****************************************************************************/
/** IRQ handler thread
 *
 * \param irq_desc	IRQ handling descriptor (IRQ number and handler 
 *			routine)
 */
/*****************************************************************************/
static void dde_irq_thread(struct irq_desc *irq_desc)
{
  unsigned int irq = irq_desc->num;	/* save irq description */
  int retval = 0;			/* thread startup return value */

  int ret, error = L4_IPC_RETIMEOUT, irq_handle;
  l4_threadid_t irq_id;
  l4_umword_t dw0, dw1;
  l4_msgdope_t result;
  unsigned int om_flags, status;
  struct irqaction *action;

  /* get permission for attaching to IRQ */
  if (use_omega0)
    {
      ret = __omega0_attach(irq, &irq_handle);
      if (ret < 0) {
#if DEBUG_IRQ
	LOG_Error("failed to attach IRQ %d at omega0!", irq);
#endif
	retval = 2;
      } else
	error = L4_IPC_RETIMEOUT;
    }
  else
    {
      if (rmgr_get_irq(irq)) {
	/* can't get permission -> block */
#if DEBUG_IRQ
	LOG_Error("%s: can't get permission for irq 0x%02x, giving up...", __FUNCTION__, irq);
#endif
	retval = 2;
      }

      /* attach to IRQ */
      if (l4_is_invalid_id(irq_id = l4util_attach_interrupt(irq))) {
#if DEBUG_IRQ
	LOG_Error("%s: can't attach to irq 0x%02x, giving up...", __FUNCTION__, irq);
#endif
	retval = 2;
      }

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

  /* we are up */
  retval += (error != L4_IPC_RETIMEOUT) ? 1 : 0;
  ret = l4thread_started(&retval);

  if ((ret < 0)|| retval) {
#if DEBUG_IRQ
    LOG_Error("IRQ%i thread startup failed!", irq);
#endif
    for (;;) {}; // Wait for termination
  }

  LOGd(DEBUG_MSG, "dde_irq_thread[%d] "l4util_idfmt" running.",
	irq_desc->num, l4util_idstr(l4thread_l4_id(l4thread_myself())));

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

      spin_lock(&handlers[irq].lock);
      if (!handlers[irq].action) {
	/* free everything and kill myself
         * in case of errors just stay and wait for recreation
	 */
	__disable_irq(irq);
	if (use_omega0) {
	    if (__omega0_detach(irq)) {
		LOG_Error("couldn't detach IRQ%i from omega0",irq);
		spin_unlock(&handlers[irq].lock);
		continue;
	    }
	} else {
	    if (rmgr_free_irq(irq)) {
		LOG_Error("couldn't detach IRQ%i from rmgr",irq);
		spin_unlock(&handlers[irq].lock);
		continue;
	    }
	}
	handlers[irq].t = 0;
	spin_unlock(&handlers[irq].lock);
	l4thread_exit();
      }
      spin_unlock(&handlers[irq].lock);

      switch (error)
	{
	case 0:
	  LOGd(DEBUG_MSG, "got IRQ %d", irq);
	  if (use_omega0)
	    om_flags = 0;
	  break;
	case L4_IPC_RETIMEOUT:
	  LOG_Error("timeout while receiving irq");
	  continue;
	default:
	  LOG_Error("receiving irq (%d)", error);
	  continue;
	}

      irq_enter();

      spin_lock(&irq_desc->lock);
      
      status = irq_desc->status & ~(IRQ_REPLAY | IRQ_WAITING);
      status |= IRQ_PENDING;
      
      action = NULL;
      if (likely(!(status & (IRQ_DISABLED | IRQ_INPROGRESS)))) {
    	    action = irq_desc->action;
	    status &= ~IRQ_PENDING;
	    status |= IRQ_INPROGRESS;
      }
      irq_desc->status = status;
      
      if (unlikely(!action))
    	    goto out;
	    
      for (;;) {
    	    spin_unlock(&irq_desc->lock);
	    
	    if (!(action->flags & SA_INTERRUPT))
		local_irq_enable();
	        status = 1;
	    do {
		status |= action->flags;
		action->handler(irq, action->dev_id, 0);
		action = action->next;
	    } while (action);
	    local_irq_disable();

	    spin_lock(&irq_desc->lock);
	    if (likely(!(irq_desc->status & IRQ_PENDING)))
		break;
	    irq_desc->status &= ~IRQ_PENDING;
      }
      irq_desc->status &= ~IRQ_INPROGRESS;

out:
      end_irq(irq);
      spin_unlock(&irq_desc->lock);
      irq_exit();
    }
}

/*****************************************************************************/
/** Request Interrupt.
 * \ingroup mod_irq
 *
 * \param  irq		interrupt number
 * \param  handler	interrupt handler -> top half
 * \param  flags	interrupt handling flags (SA_SHIRQ, ...)
 * \param  dev_name	device name
 * \param  dev_id	cookie passed back to handler
 *
 * \return 0 on success; error code otherwise
 *
 * \todo reattachment
 */
/*****************************************************************************/
int request_irq(unsigned int irq,
		irqreturn_t (*handler) (int, void *, struct pt_regs *),
		unsigned long irqflags, const char *dev_name, void *dev_id)
{
  l4thread_t irq_tid;
  struct irqaction *action, *old, **p;
  int return_v, shared = 0;
  unsigned long flags;

  if (irq >= NR_IRQS)
    return -EINVAL;
  if (!handler)
    return -EINVAL;

  action = (struct irqaction *)kmalloc(sizeof(struct irqaction), GFP_ATOMIC);
  if (!action) return -ENOMEM;

  action->handler = handler;
  action->flags = irqflags;
  action->mask = 0;
  action->name = dev_name;
  action->next = NULL;
  action->dev_id = dev_id;

  spin_lock_irqsave(&handlers[irq].lock, flags);
  p = &handlers[irq].action;
  if ((old = *p) != NULL) {
	if (!(old->flags & action->flags & SA_SHIRQ)) {
	    spin_unlock_irqrestore(&handlers[irq].lock, flags);
	    kfree(action);
	    return -EBUSY;
	}
	do {
	    p = &old->next;
	    old = *p;
	} while (old);
	shared = 1;
  }
  if (!shared) {
	handlers[irq].depth = 0;
	handlers[irq].status &= ~(IRQ_DISABLED | IRQ_AUTODETECT | IRQ_WAITING | IRQ_INPROGRESS);
  }

  if (!handlers[irq].t) {
  /* create IRQ handler thread */
	irq_tid = l4thread_create((l4thread_fn_t) dde_irq_thread,
				    (void *) &handlers[irq], L4THREAD_CREATE_SYNC);

	if (irq_tid<0)
	{
	    LOG_Error("thread creation failed");
	    kfree(action);
	    return -EAGAIN;
	}

	return_v = *(int*)l4thread_startup_return(irq_tid);
	if (return_v == 1)
	{
#if DEBUG_IRQ
	    LOG_Error("irq%i not free",irq);
#endif
	    kfree(action);
	    l4thread_shutdown(irq_tid);
	    return -EBUSY;
	} else if (return_v > 1) {
#if DEBUG_IRQ
	    LOG_Error("problems starting up irq%i",irq);
#endif
	    kfree(action);
	    l4thread_shutdown(irq_tid);
	    return -EBUSY;
	}

	handlers[irq].t = irq_tid;
  }
  
  *p = action;
  if (!shared) __enable_irq(irq);
  spin_unlock_irqrestore(&handlers[irq].lock, flags);

  LOGd(DEBUG_MSG, "attached to irq %d", irq);

  return 0;
}

/*****************************************************************************/
/** Release Interrupt
 * \ingroup mod_irq
 *
 * \param  irq		interrupt number
 * \param  dev_id	cookie passed back to handler
 *
 */
/*****************************************************************************/
void free_irq(unsigned int irq, void *dev_id)
{
    struct irqaction **p;
    unsigned long flags;
    
    if (irq >= NR_IRQS)
	return;
	
    spin_lock_irqsave(&handlers[irq].lock, flags);
    p = &handlers[irq].action;
    for (;;) {
	struct irqaction *action = *p;

	if (action) {
	    struct irqaction **pp = p;

	    p = &action->next;
	    if (action->dev_id != dev_id)
		continue;
	    *pp = action->next;
	    if (!handlers[irq].action) {
		/* The last one switches off the light */
		handlers[irq].status |= IRQ_DISABLED;
	    }
	    spin_unlock_irqrestore(&handlers[irq].lock, flags);
	    synchronize_irq(irq);
	    kfree(action);
	    return;
	}
	LOGd(DEBUG_MSG, "Trying to free free IRQ%d",irq);
	spin_unlock_irqrestore(&handlers[irq].lock, flags);
	return;
    }
}

/*****************************************************************************/
/** Request Interrupt Thread ID
 * \ingroup mod_irq
 *
 * \param  irq		interrupt number
 *
 * \return thread_id	ID of irq handler
 *
 */
/*****************************************************************************/
l4thread_t get_irq_handler_id(unsigned int irq)
{
    if (irq >= NR_IRQS)
	return L4THREAD_INVALID_ID;

    if (handlers[irq].t && handlers[irq].action)
        return handlers[irq].t;

    return L4THREAD_INVALID_ID;
}

static void end_irq(unsigned int irq)
{
    if (!(handlers[irq].status & (IRQ_DISABLED | IRQ_INPROGRESS)) && handlers[irq].action)
	__enable_irq(irq);
}

/** \name Undocumented
 * \ingroup mod_irq
 * @{ */
void disable_irq_nosync(unsigned int irq_num)
{
    unsigned long flags;
    
    spin_lock_irqsave(&handlers[irq_num].lock, flags);
    handlers[irq_num].status |= IRQ_DISABLED;
    __disable_irq(irq_num);
    spin_unlock_irqrestore(&handlers[irq_num].lock, flags);
}

void disable_irq(unsigned int irq_num)
{
    disable_irq_nosync(irq_num);
    if (handlers[irq_num].action)
	synchronize_irq(irq_num);
}

void enable_irq(unsigned int irq_num)
{
    unsigned long flags;
    unsigned int status;
    
    spin_lock_irqsave(&handlers[irq_num].lock, flags);
    status = handlers[irq_num].status & ~IRQ_DISABLED;
    handlers[irq_num].status = status;
    if ((status & (IRQ_PENDING | IRQ_REPLAY)) == IRQ_PENDING) {
	handlers[irq_num].status = status | IRQ_REPLAY;
    }
    __enable_irq(irq_num);
    spin_unlock_irqrestore(&handlers[irq_num].lock, flags);
}

/** Old probing of interrupts
 *\ingroup mod_irq
 *
 */
static DECLARE_MUTEX(probe_sem);
static int requested_irq[NR_IRQS];
static irqreturn_t probe_handler(int irq, void *id, struct pt_regs *pt)
{
    return 0;
}

/**
 *	probe_irq_on	- begin an interrupt autodetect
 *
 *	Commence probing for an interrupt. The interrupts are scanned
 *	and a mask of potential interrupt lines is returned.
 *
 */
 
unsigned long probe_irq_on(void)
{
	unsigned int i;
	irq_desc_t *desc;
	unsigned long val;
	unsigned long delay;

	down(&probe_sem);
	/* 
	 * something may have generated an irq long ago and we want to
	 * flush such a longstanding irq before considering it as spurious. 
	 */
	for (i = NR_IRQS-1; i > 0; i--)  {
		desc = &handlers[i];
		requested_irq[i] = 0;

		spin_lock_irq(&desc->lock);
		if (!desc->action) {
		    if (request_irq(i, probe_handler, 0, "__probe_irq", 0) == 0) {
			requested_irq[i] = 1;
		    }
		}
		spin_unlock_irq(&desc->lock);
	}

	/* Wait for longstanding interrupts to trigger. */
	for (delay = jiffies + HZ/50; time_after(delay, jiffies); )
		/* about 20ms delay */ barrier();

	/*
	 * enable any unassigned irqs
	 * (we must startup again here because if a longstanding irq
	 * happened in the previous stage, it may have masked itself)
	 */
	for (i = NR_IRQS-1; i > 0; i--) {
		desc = &handlers[i];

		spin_lock_irq(&desc->lock);
		if (requested_irq[i]) {
			desc->status |= IRQ_AUTODETECT | IRQ_WAITING;
		}
		spin_unlock_irq(&desc->lock);
	}

	/*
	 * Wait for spurious interrupts to trigger
	 */
	for (delay = jiffies + HZ/10; time_after(delay, jiffies); )
		/* about 100ms delay */ barrier();

	/*
	 * Now filter out any obviously spurious interrupts
	 */
	val = 0;
	for (i = 0; i < NR_IRQS; i++) {
		irq_desc_t *desc = &handlers[i];
		unsigned int status;

		spin_lock_irq(&desc->lock);
		status = desc->status;

		if ((status & IRQ_AUTODETECT) && requested_irq[i]) {
			/* It triggered already - consider it spurious. */
			if (!(status & IRQ_WAITING)) {
				desc->status = status & ~IRQ_AUTODETECT;
				free_irq(i, 0);
				requested_irq[i] = 0;
			} else
				if (i < 32)
					val |= 1 << i;
		}
		spin_unlock_irq(&desc->lock);
	}

	return val;
}

/*
 * Return a mask of triggered interrupts (this
 * can handle only legacy ISA interrupts).
 */
 
/**
 *	probe_irq_mask - scan a bitmap of interrupt lines
 *	@val:	mask of interrupts to consider
 *
 *	Scan the ISA bus interrupt lines and return a bitmap of
 *	active interrupts. The interrupt probe logic state is then
 *	returned to its previous value.
 *
 *	Note: we need to scan all the irq's even though we will
 *	only return ISA irq numbers - just so that we reset them
 *	all to a known state.
 *
 * /todo eventually free irqs requested in probe_irq_on
 */
unsigned int probe_irq_mask(unsigned long val)
{
	int i;
	unsigned int mask;

	mask = 0;
	for (i = 0; i < NR_IRQS; i++) {
		irq_desc_t *desc = &handlers[i];
		unsigned int status;

		spin_lock_irq(&desc->lock);
		status = desc->status;

		if (status & IRQ_AUTODETECT) {
			if (i < 16 && !(status & IRQ_WAITING))
				mask |= 1 << i;

			desc->status = status & ~IRQ_AUTODETECT;
			__disable_irq(i);
		}
		spin_unlock_irq(&desc->lock);
	}
	up(&probe_sem);

	return mask & val;
}

/*
 * Return the one interrupt that triggered (this can
 * handle any interrupt source).
 */

/**
 *	probe_irq_off	- end an interrupt autodetect
 *	@val: mask of potential interrupts (unused)
 *
 *	Scans the unused interrupt lines and returns the line which
 *	appears to have triggered the interrupt. If no interrupt was
 *	found then zero is returned. If more than one interrupt is
 *	found then minus the first candidate is returned to indicate
 *	their is doubt.
 *
 *	The interrupt probe logic state is returned to its previous
 *	value.
 *
 *	BUGS: When used in a module (which arguably shouldnt happen)
 *	nothing prevents two IRQ probe callers from overlapping. The
 *	results of this are non-optimal.
 */
 
int probe_irq_off(unsigned long val)
{
	int i, irq_found, nr_irqs;

	nr_irqs = 0;
	irq_found = 0;
	for (i = 0; i < NR_IRQS; i++) {
		irq_desc_t *desc = &handlers[i];
		unsigned int status;

		spin_lock_irq(&desc->lock);
		status = desc->status;

		if ((status & IRQ_AUTODETECT) && requested_irq[i]) {
			if (!(status & IRQ_WAITING)) {
				if (!nr_irqs)
					irq_found = i;
				nr_irqs++;
			}
			desc->status = status & ~IRQ_AUTODETECT;
		}
		spin_unlock_irq(&desc->lock);
		if (requested_irq[i])
		 free_irq(i, 0);
	}
	up(&probe_sem);

	if (nr_irqs > 1)
		irq_found = -irq_found;
	return irq_found;
}

/** @} */
/*****************************************************************************/
/** Initalize IRQ handling
 * \ingroup mod_irq
 *
 * \param omega0	If set use Omega0 interrupt handling - if unset use
 *			RMGR directly
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int l4dde_irq_init(int omega0)
{
  int i;

  if (_initialized)
    return -L4_ESKIPPED;

  use_omega0 = omega0;

  memset(&handlers, 0, sizeof(handlers));
  for (i=0; i<NR_IRQS;i++) {
    handlers[i].num = i;
    handlers[i].status = IRQ_DISABLED;
    handlers[i].lock = SPIN_LOCK_UNLOCKED;
  }

  preempt_count() = 0;
  l4util_sti();
  ++_initialized;
  return 0;
}
