#ifndef __OMEGA0_SERVER_CONFIG_H
#define __OMEGA0_SERVER_CONFIG_H

// set to 1 if you want verbose output in irq_threads.c
#define OMEGA0_DEBUG_IRQ_THREADS 0

// set to 1 if you want logging of user requests
#define OMEGA0_DEBUG_REQUESTS 0

// set to 1 if you want verbose output of pic-related actions
#define OMEGA0_DEBUG_PIC 0

// se to 1 if you want the lowest 32bit of TSC in dw1 when sending an
// IPC to clients
#define OMEGA0_DEBUG_MEASUREMENT_SENDTIME 0

// set to 1 if you want verbose startup
#define OMEGA0_DEBUG_STARTUP 0

/* set to 1 if you want the server to enter-kdebug on fairly hard
 * errors (like memory shortage or other unexpected things). */
#define ENTER_KDEBUG_ON_ERRORS 1

/* set to 1 to print error messages about invalid requests or
 * failed wakeups of client */
#define OMEGA0_DEBUG_WARNING 0

/* set to 1 if you want the server to automatically consume shared
 * irqs if all clients wait for the irq. */
#define OMEGA0_STRATEGY_AUTO_CONSUME 1

/* set to 1 if you want implicit masking and acknowledging of irqs. 
 * See paper on omega0, section 4.3 "Policies", Strategy 1. */
#define OMEGA0_STRATEGY_IMPLICIT_MASK 1

/* set to 1 if you want to use specific EOI sequence. Otherwise
 * unspecific end-of-int sequence. The latter results in prio
 * requirements, the handlings threads must be either in the same
 * prios as the hardware is, or they must run with disabled irqs until
 * acknowledge proceeded. */
#define OMEGA0_STRATEGY_SPECIFIC_EOI 1

/* define the following if you want explicit locking around
 * PIC-accesses. The locking is not needed on single-cpu-boards
 * because we use cli/sti-pairs around the port accesses. For
 * SMP-machines, we need a completely new omega0-server. */
#define OMEGA0_USE_PIC_LOCKING 0


#endif
