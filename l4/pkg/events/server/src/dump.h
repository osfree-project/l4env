#include "types.h"

#ifndef __L4EVENTS_DUMP
#define __L4EVENTS_DUMP

void 
dump_channel_list(channel_item_t *curr_channel);

void 
dump_task_list(task_item_t *curr_task);
  
void 
dump_ack_list(ack_item_t *curr_ack);

void 
dump_timeout_queue(event_item_t *timeout_curr_event);

#endif
