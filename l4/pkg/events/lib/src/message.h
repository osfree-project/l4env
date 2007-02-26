#ifndef L4EVENTS_LIB_MESSAGE_H
#define L4EVENTS_LIB_MESSAGE_H

extern l4_threadid_t l4events_server;

extern long l4events_send_message(message_t* msg, l4_timeout_t timeout);
extern long l4events_send_short_message(l4_umword_t *w1, l4_umword_t *w2,
					l4_timeout_t timeout);
extern long l4events_send_short_open_message(l4_threadid_t *id,
    					     l4_umword_t *w1, l4_umword_t *w2,
					     l4_timeout_t timeout);
extern long l4events_send_recv_message (l4_umword_t w1, message_t* msg, 
					l4_timeout_t timeout);

#endif

