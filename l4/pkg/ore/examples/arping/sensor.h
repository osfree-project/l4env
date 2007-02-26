/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#ifndef __SENSOR_EVENTS_H
#define __SENSOR_EVENTS_H

#include <l4/ferret/client.h>
#include <l4/ferret/clock.h>
#include <l4/ferret/types.h>
#include <l4/ferret/util.h>
#include <l4/ferret/sensors/list_producer.h>

/* This is going to be the common header of all events. */
typedef struct __attribute__((__packed__))
{
    l4_cpu_time_t   timestamp;      // CPU timestamp
    l4_uint16_t     major;          // the next 3 make up the guid later
    l4_uint16_t     minor;
    l4_uint16_t     instance;
    l4_uint8_t      cpu;
    l4_uint16_t     version;        
    l4_uint16_t     type;       
    l4_uint16_t     taskno;         // event producer
    l4_uint16_t     threadno;       // ----- " ------
} common_event_header;

typedef struct __attribute__((__packed__))
{
} arping_empty_event;

/* IPC event - this is built in 2 steps:
 *  (1) before a call to ORe, set handle and start_ts
 *  (2) after return from call, set return value and stop_ts
 */
typedef struct __attribute__((__packed__))
{
    l4_uint32_t     handle;
    l4_uint32_t     data_ptr;
    l4_uint32_t     size_send;          // send data size
    l4_uint32_t     size_recv;          // recv data size
    l4_uint32_t     return_value;
    l4_uint8_t      flags;
    l4_cpu_time_t   start_ts;
    l4_cpu_time_t   stop_ts;
} arping_rxtx_event;

// a smaller version of the rxtx event
typedef struct __attribute__((__packed__))
{
    l4_uint32_t     handle;
    l4_cpu_time_t   start_ts;
    l4_cpu_time_t   stop_ts;
} arping_rxtx_fast_event;

/* IPC event flags */
#define ARPING_IPC_SEND_FLAG    1
#define ARPING_IPC_RECV_FLAG    2

typedef struct __attribute__((__packed__))
{
    l4_cpu_time_t   start_ts;
    l4_cpu_time_t   stop_ts;
} arping_void_function_event;

typedef struct __attribute__((__packed__))
{
    common_event_header     head;
    union
    {
        arping_empty_event          empty;
        arping_rxtx_event           ipc;
        arping_rxtx_fast_event      ipc_fast;
        arping_void_function_event  void_call;
    };
} arping_event;

/* Fill the event header with static information. */
void fill_event_header(common_event_header *h, l4_uint16_t major,
        l4_uint16_t minor, l4_uint16_t instance, l4_uint16_t cpu,
        l4_uint16_t version, l4_uint16_t task, l4_uint16_t thread);       
void fill_event_header( common_event_header *h, 
                        l4_uint16_t major,
                        l4_uint16_t minor, 
                        l4_uint16_t instance, 
                        l4_uint16_t cpu,
                        l4_uint16_t version, 
                        l4_uint16_t task,
                        l4_uint16_t thread)
{
    h->timestamp    = 0;            // timestamp cannot be set statically
    h->major        = major;
    h->minor        = minor;
    h->instance     = instance;
    h->cpu          = cpu;
    h->version      = version;
    h->type         = 0;            // type is determined later
    h->taskno       = task;
    h->threadno     = thread;
}

/* Sensor major minor numbers */
#define ARPING_SENSOR_MAJOR     1
#define ARPING_SENSOR_MINOR     1

/* version number */
#define ARPING_EVENT_VERSION       0

/* event types */
/* Q: Why don't we have a send and a receive event?
 * A: Because then we also needed a send_receive event. With the single
 *    IPC event we can use the event's flag field to signal that it is
 *    a send or receive event or both.
 */
#define ARPING_IPC_EVENT            1
#define ARPING_PARSE_EVENT          2
#define ARPING_CREATE_REPLY_EVENT   3

#endif
