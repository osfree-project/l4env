#ifndef __L4_THREAD_H
#define __L4_THREAD_H

l4_threadid_t create_thread (int thread_no, void (*function)(void), 
			     int *stack);

l4_threadid_t attach_interrupt (int irq);
void detach_interrupt (void);


#endif /* __L4_THREAD_H */
