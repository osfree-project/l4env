/*!
 * \file   omega0/include/client.h
 * \brief  Omega0 client interface
 *
 * \date   2000/02/08
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2000-2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __OMEGA0_INCLUDE_CLIENT_H_
#define __OMEGA0_INCLUDE_CLIENT_H_

#include <l4/sys/types.h>

/*!\brief Omega0 IRQ descriptor, bitfields
 *\ingroup clientapi
 */
typedef struct omega0_irqdesc_struct_t{
  l4_umword_t num      :16;
  l4_umword_t shared   :1;
  l4_umword_t reserved :15;
} omega0_irqdesc_struct_t;

/*!\brief Omega0 IRQ descriptor
 *\ingroup clientapi
 */
typedef union omega0_irqdesc_t{
  omega0_irqdesc_struct_t s;
  l4_umword_t i;
} omega0_irqdesc_t;


/*!\brief Omega0 request type, bitfields
 *\ingroup clientapi
 */
typedef struct omega0_request_struct_t{
  unsigned	param:16;
  unsigned	wait:1;
  unsigned	consume:1;
  unsigned	mask:1;
  unsigned	unmask:1;
  unsigned	again:1;
  unsigned	reserved:11;
} omega0_request_struct_t;

/*!\brief Omega0 request type
 *\ingroup clientapi
 */
typedef union omega0_request_t{
  omega0_request_struct_t s;
  l4_umword_t i;
} omega0_request_t;

/*!\brief Omega0 alien handler type
 *\ingroup clientapi
 */
typedef void(*omega0_alien_handler_t)(l4_threadid_t alien, l4_umword_t d0,
                                      l4_umword_t d1);
extern omega0_alien_handler_t omega0_alien_handler;

// constants for creating OMEGA0_RQ
#define OMEGA0_WAIT	(((omega0_request_t){s:			\
                             (omega0_request_struct_t){wait:1}}).i)
#define OMEGA0_CONSUME	(((omega0_request_t){s:			\
                             (omega0_request_struct_t){consume:1}}).i)
#define OMEGA0_MASK	(((omega0_request_t){s:			\
                             (omega0_request_struct_t){mask:1}}).i)
#define OMEGA0_UNMASK	(((omega0_request_t){s:			\
                             (omega0_request_struct_t){unmask:1}}).i)

#define OMEGA0_RQ(action,param) ((omega0_request_t){i:(param)|(action)})

/*!\brief Attach to an irq line.
 *\ingroup clientapi
 *
 * \param  desc			IRQ descriptor
 * \param  desc.s.i_num		IRQ line
 * \param  desc.s.shared	1 if the client is willing to share the IRQ
 * \retval >=0			success: handle
 * \retval <0			error
 */
extern int omega0_attach(omega0_irqdesc_t desc);

/*!\brief Detach from an irq line.
 *\ingroup clientapi
 *
 * \param  desc			IRQ descriptor
 * \retval 0			success
 * \retval <0			error
 */
extern int omega0_detach(omega0_irqdesc_t desc);

/*!\brief Pass the right to attach to an IRQ line to another thread.
 *\ingroup clientapi
 *
 * \param  desc			IRQ descriptor, .s.i_num: irq line
 * \param  new_driver		new thread
 *
 * \retval 0			success
 * \retval <0			error
 */
extern int omega0_pass(omega0_irqdesc_t desc, l4_threadid_t new_driver);

/*!\brief Request for certain actions.
 *\ingroup clientapi
 *
 * \param  handle		handle returned from omega0_attach()
 * \param  action		combination of bits indicating
 *				the indicated actions. the meaning of
 *				action.param depends on the actions.
 *
 * \retval 0			success
 * \retval <0			error
 * \retval >0			If the wait option was set, indicates
 *				the irq line.
 */
extern int omega0_request(int handle, omega0_request_t action);


/*!\brief Request for certain actions with timeout.
 *\ingroup clientapi
 *
 * \param  handle		handle returned from omega0_attach()
 * \param  action		combination of bits indicating
 *				the indicated actions. the meaning of
 *				action.param depends on the actions.
 * \param  timeout		L4 timeout
 *
 * \retval 0			success
 * \retval <0			error
 * \retval >0			If the wait option was set, indicates
 *				the irq line.
 */
extern int omega0_request_timeout(int handle, omega0_request_t action,
				  l4_timeout_t timeout);


/*!\brief Iterator: return the first available irq line.
 *\ingroup clientapi
 *
 * \retval 0			no irq lines are available
 * \retval >0			first existing irq line.
 */
extern int omega0_first(void);

/*!\brief Iterator: return next available irq line.
 *
 * \param  irq_num		return value of omega0_first() or a
 *				omega0_next()
 * \retval 0			no more lines are available
 * \retval >0			next existing irq line.
 */
extern int omega0_next(int irq_num);

/*!\brief Set an alien IPC handler
 *\ingroup clientapi
 *
 * If a request (omega0_request()) with a set waiting-flag is not
 * answered by the omega0 server but by someone else (the alien), a
 * callback may be called. Therefore, first register the handler using
 * this function and call omega0_request() then. The alien IPC must be
 * a short IPC without any mappings. The alien handler receives the
 * alien thread id and the 2 (two) dwords.
 */
extern omega0_alien_handler_t omega0_set_alien_handler(
    omega0_alien_handler_t handler);


#endif
