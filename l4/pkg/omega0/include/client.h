#ifndef __OMEGA0_CLIENT_H
#define __OMEGA0_CLIENT_H

#include <l4/sys/types.h>

typedef struct{
  unsigned	num	:16;
  unsigned	shared:1;
  unsigned	reserved:15;
} omega0_irqdesc_struct_t;

typedef union{
  omega0_irqdesc_struct_t s;
  unsigned i;
} omega0_irqdesc_t;


typedef struct{
  unsigned	param:16;
  unsigned	wait:1;
  unsigned	consume:1;
  unsigned	mask:1;
  unsigned	unmask:1;
  unsigned	reserved:12;
} omega0_request_struct_t;

typedef union{
  omega0_request_struct_t s;
  unsigned i;
} omega0_request_t;


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

/* attach to an irq line. desc.s.i_num denotes the irq line, desc.s.shared
   denotes if the client is willing to use shared irq's. return value is
   a non-negative handle on success or negativ on failure. */
extern int omega0_attach(omega0_irqdesc_t desc);

/* detach from an irq line. return value of 0 indicates success, negative
   ones indicate failures. */
extern int omega0_detach(omega0_irqdesc_t desc);

/* pass the right to attach to an IRQ line to thread new_driver. return value
   of 0 indicates success, negative ones failures. */
extern int omega0_pass(omega0_irqdesc_t desc, l4_threadid_t new_driver);


/* request for certain actions. action holds a combination of bits indicating
   the indicated actions. the meaning of action.param depends on the
   actions. negative return values indicate errors, 0 success. if the wait
   option is set, positive return value is the number of irq line. */
extern int omega0_request(int handle, omega0_request_t action);


/* return the first available irq line. returns 0 if no irq lines are
   available, otherwise the first one existing. */
extern int omega0_first(void);

/* return the next available irq line. returns 0 if no more lines are
   available, positive number else. */
extern int omega0_next(int irq_num);

#endif
