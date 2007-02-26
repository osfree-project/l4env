#include <l4/events/events.h>
#include <l4/sys/types.h>
#include "globals.h"

#ifndef __L4EVENTS_TYPES
#define __L4EVENTS_TYPES

/* some forward declaration */
struct event_item;
struct task_item;
struct channel_item;
struct notify_task_item;

/* some simple linked lists */
struct event_ref {
  struct event_item	*event_item;
  struct event_ref	*next_ref;
} ;

struct task_ref {
  struct task_item	*task_item;
  struct task_ref	*next_ref;
} ;

struct channel_ref {
  struct channel_item	*channel_item;
  struct channel_ref	*next_ref;
} ;

    
typedef struct event_ref	event_ref_t;
typedef struct task_ref		task_ref_t;
typedef struct channel_ref	channel_ref_t;


struct event_item {
  /* event_ch of this event */
  l4events_ch_t		event_ch;
  /* unique event_nr for sending acknowledgement */
  l4events_nr_t		event_nr;
  /* number of blocked tasks */
  unsigned int		blocked_tasks;
  /* number of pending tasks not received this event */
  unsigned int		pending_tasks;
  /* number of tasks not acknowledged this event */
  unsigned int		ack_tasks;

  /* reference to the associated channel */
  struct channel_item	*channel;
  /* next priority class to send to 
   * >= 0	some lower priority task wants to receive
   * = -1 	no other lower priority tasks to send*/
  l4events_pr_t		priority;
  
  /* relalive timeout for sending the event to the next lower priority class
   * >= 0	event is in timeout queue 
   * = -1	event is NOT in timeout queue */
  l4_int8_t		timeout;
  /* timeout list */
  struct event_item	*timeout_prev_event;
  struct event_item	*timeout_next_event;
  
  /* the event */
  l4events_event_t	*event;
} ;


struct task_item {
  /* task id */
  l4_umword_t		taskid;
  /* number of registered channels */
  l4_umword_t		registered_channels;
  /* number of blocked events */
  l4_umword_t		blocked_events;
  /* number of pending events this task not received yet */
  l4_umword_t		pending_events;
  /* number of events to acknowledge after receive  */
  l4_umword_t		ack_events;
  /* a thread of this task is waiting for event */
  l4_threadid_t		waiting_threadid;
  /* the maximum length of the event waiting for
     this is important for short IPC!!! */
  l4_uint16_t		waiting_length;
  /* the channel the client wants to receive */
  l4_uint8_t		waiting_event_ch;
  /* giving acknowledge after process event */
  l4_uint8_t		waiting_ack;
  /* reference to first channel this task is registered for */
  struct channel_ref	*first_channel_ref;

  /* blocked-queue for events of this task */
  /* reference to first blocked event */
  struct event_ref	*first_blocked_event_ref;
  /* reference to last blocked event */
  struct event_ref	*last_blocked_event_ref;
    
  /* pending-queue for events of this task */
  /* reference to first pending event to receive  */
  struct event_ref	*first_pending_event_ref;
  /* reference to last pending event to receive */
  struct event_ref	*last_pending_event_ref;

  /* ack-queue for events of this task */
  /* reference to first event for which to give acknowledge */
  struct event_ref	*first_ack_event_ref;
  /* reference to last event for which to give acknowledge */
  struct event_ref	*last_ack_event_ref;

  /* next task in list */
  struct task_item	*next_item;
} ;

/* defines the stucture for an channel */
struct channel_item {
  l4events_ch_t		event_ch;
  /* number of registered tasks */
  l4_umword_t		registered_tasks;
  /* refrence to first task registered for this channel */
  struct task_ref	*first_task_ref[L4EVENTS_MAX_PRIORITY+1];

  /* next channel in list */
  struct channel_item	*next_item;
} ;

/* defines the structure for giving acknowledge to a sending task */
struct ack_item {
    /* threadid of the sender */
    l4_threadid_t	senderid;
    /* threadid of the waiting thread 
     * L4_INALID_ID	event is NOT ready for ack
     * L4_NIL_ID	event is ready for ack
     * else 		thread is waiting for ack */
    l4_threadid_t	threadid;
    /* evenid */
    l4events_ch_t	event_ch;
    /* event_nr */
    l4events_nr_t	event_nr;
    /* next notify_task in list */
    struct ack_item	*next_item;
} ;

typedef struct event_item event_item_t;
typedef struct task_item task_item_t;
typedef struct channel_item channel_item_t;
typedef struct ack_item ack_item_t;


#endif

