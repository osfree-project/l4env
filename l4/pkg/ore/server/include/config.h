/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#ifndef __CONFIG_H
#define __CONFIG_H

// Memory that is mapped to the Linux emulation
#define ORE_LINUXEMUL_MEMSIZE         (16*1024*1024)

// priority for the IRQ handler thread
#define IRQ_HANDLER_PRIO              0xF0

// maximum number of connections one instance of ORe can handle
#define ORE_CONFIG_MAX_CONNECTIONS    30
// maximum size of a packet
#define ORE_CONFIG_MAX_BUF_SIZE       16500

// flags for server-internal state
#define ORE_FLAG_RX_WAITING           1

#define ERRLOG(err) if ((err)) {                                           \
			LOG_Error("%d (%s)", (err), l4env_errstr((-err))); \
			return err; }

#define LOG_SKB(s) { if ((s) != NULL)                                       \
                     {      LOG("skb            = %p", (s));                \
                            LOG("skb->len       = %d", (s)->len);           \
                            LOG("skb->data_len  = %d", (s)->data_len);      \
                            LOG("skb->head      = %p", (s)->head);          \
                            LOG("skb->data      = %p", (s)->data);          \
                            LOG("skb->tail      = %p", (s)->tail);          \
                            LOG("skb->end       = %p", (s)->end);           \
                     }                                                      \
                     else LOG("skb == NULL");                               \
                   }

/* DEBUG stuff */
extern int ore_debug;

// default value of ore_debug
#define ORE_DEBUG_DEFFAULT            0

// default debug output
#define ORE_DEBUG                     (0 && ore_debug)
// debug output for initialization routines
#define ORE_DEBUG_INIT                (0 && ore_debug)
// debug output for server components
#define ORE_DEBUG_COMPONENTS          (0 && ore_debug)

// These three are the most verbose debug messages - use with care
// debug packet send path
#define ORE_DEBUG_PACKET_SEND         (0 && ore_debug)
// debug packet receive path
#define ORE_DEBUG_PACKET_RECV         (0 && ore_debug)
// debug all packet paths
#define ORE_DEBUG_PACKET              (ORE_DEBUG_PACKET_SEND && \
                                       ORE_DEBUG_PACKET_RECV && \
                                       ore_debug)
// debug output for linux emulation
#define ORE_EMUL_DEBUG                (0 && ore_debug)
// debug interrupt handling
#define ORE_DEBUG_IRQ                 (0 && ore_debug)
// debug event handling
#define ORE_DEBUG_EVENTS              (0 && ore_debug)
// debug DSI stuff
#define ORE_DEBUG_DSI                 (0 && ore_debug)

#endif /* ! __CONFIG_H */
