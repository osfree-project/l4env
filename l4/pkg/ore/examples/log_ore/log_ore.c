/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include "uip.h"
#include "uip_arp.h"
#include "muxed.h"

#include <l4/thread/thread.h>
#include <l4/log/l4log.h>
#include <l4/lock/lock.h>
#include <l4/sys/ipc.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/util/macros.h>

#include <netinet/in.h>

#define BUF                 ((struct uip_eth_hdr *)&uip_buf[0])

l4_threadid_t log_ore_thread    = L4_INVALID_ID;
unsigned client_socket          = 0;

l4lock_t __log_ore_rxtx_lock    = L4LOCK_UNLOCKED;
LIST_HEAD(__log_ore_rx_list);
LIST_HEAD(__log_ore_tx_list);

static int send_data_if_available(struct log_state *);
    
void log_ore_init(void)
{
    int ret = l4thread_create(log_ore_threadfunc, NULL, L4THREAD_CREATE_SYNC);
}

/* Send the next rxtx entry if one is available in the tx_list.
 *
 * Return 0, if no data was sent, 1 if send operation was started.
 */
static int send_data_if_available(struct log_state *state)
{
    struct rxtx_entry *e;
    char buf[256];

    l4lock_lock(&__log_ore_rxtx_lock);
    if (list_empty(&__log_ore_tx_list))
    {       
        if (debug)
            outstring("NO DATA TO SEND\n");
        l4lock_unlock(&__log_ore_rxtx_lock);
        return 0; 
    }       

    e = list_entry(__log_ore_tx_list.next, struct rxtx_entry, list);
    if (e) // XXX: paranoid!
        list_del(&e->list);
    l4lock_unlock(&__log_ore_rxtx_lock);

    if (e)  
    {   
        if (debug)
            outstring("sending data\n");
#if BLUB // not multiplexed
        snprintf(buf, 256, "\033[037m%s\033[0m\n", e->buf);
#else
        snprintf(buf, 256, "%s", e->buf);
#endif
        uip_send(buf, strlen(buf));
        state->state = WAIT_ACK;
        if (e->buf)
            free(e->buf);
        free(e);        
    }

    return 1;
}

void log_ore_callback(void)
{
    char buf[256];
    if (debug)
        outstring("callback\n");
    if (uip_conn->lport == HTONS(port_nr))
    {
        struct log_state *state = (struct log_state *)(uip_conn->appstate);
       
        if (debug)
            outstring("correct port\n");

        if (uip_connected())
        {
            struct in_addr client_ip;
            
            if (debug)
                outstring("conn\n");
            state->state    = WAIT_ACK;

            client_ip = *(struct in_addr *)(uip_conn->ripaddr);
            LOG("connection from: %s", inet_ntoa(client_ip));
            if (!binary)
            {
                sprintf(buf, "\033[32mHello from the log server!\033[0m\n");
                uip_send(buf, strlen(buf));
            }
            else
            {
                // TODO: send binary welcome message...
            }
        }
        else if (uip_poll() && state->state == LOG_READY)
        {
            struct rxtx_entry *e;
            if (debug)
                outstring("poll\n");
            // send data if available
            send_data_if_available(state);
        }
        else if (uip_newdata())
        {
            if (debug)
                outstring("new data\n");
            // flush request from client
        }

        if (uip_acked())
        {
            if (debug)
                outstring("ack\n");
            // we received an ack message - go sending the next one
            if (send_data_if_available(state) == 0)
                state->state = LOG_READY;
        }
    }
}

void log_ore_threadfunc(void *p)
{
    short arptimer=0;
    struct in_addr my_in;
    unsigned short ip[2];
    int ret;
    char buf[256];
    
    uip_init();
    oredev_init();
    log_ore_thread = l4_myself();

    // TODO: init IP address
    ret = inet_aton(ip_addr, &my_in);
    if (ret)
    {
        if (debug)
        {
            snprintf(buf, 256, "setting IP address to %s\n", ip_addr);
            outstring(buf);
        }
        memcpy(ip, &my_in.s_addr, 4);
        uip_sethostaddr(ip);
    }
    else
    {
        snprintf(buf, 256, "Error parsing IP address %s\n", ip_addr);
        outstring(buf);
    }

    if (debug)
    {
        snprintf(buf, 256, "listening on port %d (%d)\n", port_nr, HTONS(port_nr));
        outstring(buf);
    }
    uip_listen(HTONS(port_nr));

    l4thread_started(NULL);
    outstring("log_ore uIP thread is up and running.\n");
    
    while (1)
    {
        uip_len = oredev_read();
        if (uip_len == 0)
        {
            int i;
            for (i=0; i < UIP_CONNS; i++)
            {
                uip_periodic(i);
                if (uip_len > 0)
                {
                    uip_arp_out();
                    oredev_send();
                }
            }
        }
        else
        {
            if (BUF->type == htons(UIP_ETHTYPE_IP))
            {
                uip_arp_ipin();
                uip_input();
                if (uip_len > 0)
                {
                    uip_arp_out();
                    oredev_send();
                }
            }
            else if (BUF->type == htons(UIP_ETHTYPE_ARP))
            {
                uip_arp_arpin();
                if (uip_len > 0)
                {
                    oredev_send();
                }
            }
        }

        if (++arptimer == 20)
        {
            uip_arp_timer;
            arptimer = 0;
        }
    }
}

void log_ore_flush_buffer(void)
{
    int i;

    for (i=0; i<MAX_BIN_CONNS; i++)
    {
        if (bin_conns[i].channel &&
            bin_conns[i].flushed < bin_conns[i].written)
        {
            log_ore_send_to_channel(bin_conns[i].addr, bin_conns[i].size,
                    bin_conns[i].channel);
            bin_conns[i].flushed++;
        }
    }
}

void log_ore_send_to_channel(void *addr, int size, int channel)
{
    struct muxed_header *header = malloc(sizeof(struct muxed_header));
    struct rxtx_entry *e1 = malloc(sizeof(struct rxtx_entry));
    struct rxtx_entry *e2 = malloc(sizeof(struct rxtx_entry));
    int ret;

    header->version      = LOG_PROTOCOL_VERSION;
    header->channel      = channel;
    header->type         = LOG_TYPE_LOG;
    header->flags        = LOG_FLAG_NONE;
    header->data_length  = size;
    
    e1->buf = (void *)header;
    e1->size = sizeof(struct muxed_header);
    e2->buf = addr;
    e2->size = size;

    l4lock_lock(&__log_ore_rxtx_lock);
    list_add_tail(&e1->list, &__log_ore_tx_list);
    list_add_tail(&e2->list, &__log_ore_tx_list);
    l4lock_unlock(&__log_ore_rxtx_lock);
}
