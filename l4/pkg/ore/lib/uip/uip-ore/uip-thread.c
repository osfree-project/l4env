/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

#include "uip.h"
#include "uip_arp.h"
#include <arpa/inet.h>
#include <l4/sys/kdebug.h>
#include <l4/ore/uip-ore.h>
#include <stdio.h>
#ifdef SENSOR
#include <l4/ferret/client.h>
#include <l4/ferret/types.h>
#include <l4/ferret/sensors/scalar_producer.h>

ferret_scalar_t *sc;
long long sc_cnt = 0;
#endif

struct uip_ore_config __uip_ore_config;
static char *last_send_buffer[MAX_CONN];
static unsigned last_send_size[MAX_CONN];

#define BUF          ((struct uip_eth_hdr *)&uip_buf[0])

/* Send a packet if there is one in the tx_list. 
 *
 * \retval  0   no packet to send
 * \retval  1   successfully sent packet
 */
static int send_data_if_available(unsigned port);
static int send_data_if_available(unsigned port)
{
    struct rxtx_entry *e;
    short idx = get_id_for_port(port);

    l4lock_lock(&conn_table[idx].__listlock);
    if (list_empty(&conn_table[idx].__tx_list))
    {
        l4lock_unlock(&conn_table[idx].__listlock);
        return 0; 
    }       

    e = list_entry(conn_table[idx].__tx_list.next, struct rxtx_entry, list);
    if (e) 
        list_del(&e->list);
    l4lock_unlock(&conn_table[idx].__listlock);

    if (e)  
    {    
        // schedule buffer to be sent
        uip_send(e->buf, e->size);
        // remember address of buffer. note that uIP only supports
        // sending of one buffer at a time, so we do not need to
        // care for more.
        last_send_buffer[idx] = e->buf;
        last_send_size[idx]   = e->size;
        // free the rxtx_entry
        free(e);        
    }

    return 1;
}

/* Callback function. It is called by the uIP stack whenever an event occurs
 * that needs to be handled by user interaction.
 */
void uip_ore_callback(void)
{
    char buf[128];
    unsigned short port = HTONS(uip_conn->lport);
    
    struct uip_connstate *state = (struct uip_connstate *)(uip_conn->appstate);

    if (uip_poll() && __uip_ore_config.poll_callback != NULL)
        __uip_ore_config.poll_callback();

    /* check if we need to close the connection. */
    int idx = get_id_for_port(port);
    if (idx >= 0 && idx < MAX_CONN && conn_table[idx].close_flag)
    {
        uip_close();
        conn_table[idx].close_flag = 0;
        return;
    }

   // periodic poll from uIP AND we are ready to send another packet
    if (uip_poll() && state->state == UIP_READY) 
    {   
        if (send_data_if_available(port) == 0)
        {
            state->state = UIP_READY;
        }
        else
        {
            state->state = UIP_BUSY;
        }
    }

    // new connection
    if (uip_connected())
    {
        struct in_addr client_ip = *(struct in_addr *)(uip_conn->ripaddr);
//            snprintf(buf, 128, "connection from %s\n", inet_ntoa(client_ip));
//            outstring(buf);

        // just in case no one added this port up to now...
        add_port(port);
        if (__uip_ore_config.connect_callback != NULL)
            __uip_ore_config.connect_callback(client_ip, port);
        state->state = UIP_READY;
    }
    // received a packet acknowledgement
    if (uip_acked())
    {
        short idx = get_id_for_port(port);
        if (idx >= 0 && __uip_ore_config.ack_callback != NULL)
        {
            __uip_ore_config.ack_callback(last_send_buffer[idx], port);
            // Be cool, keep clean!
            last_send_buffer[idx] = NULL;
            last_send_size[idx]   = 0;
            state->state = UIP_READY;
            if (send_data_if_available(port) == 0)
            {
                state->state = UIP_READY;
            }
            else
            {
                state->state = UIP_BUSY;
            }
        }
    }
    // new data from client arrived
    if (uip_newdata())
    {
        if (__uip_ore_config.recv_callback != NULL)
            __uip_ore_config.recv_callback((void *)uip_appdata, uip_datalen(), port);
    }
     // connection closed
    if (uip_closed())
    {
        cleanup_connection(port);
        if (__uip_ore_config.close_callback != NULL)
            __uip_ore_config.close_callback(port);
    }

    // retransmit packet
    if (uip_rexmit())
    {
        short idx = get_id_for_port(port);
        if (idx >= 0 && __uip_ore_config.rexmit_callback != NULL)
        {
            __uip_ore_config.rexmit_callback(last_send_buffer[idx], last_send_size[idx], port);
        }
    }

    // connection aborted
    if (uip_aborted())
    {
        if (__uip_ore_config.abort_callback != NULL)
            __uip_ore_config.abort_callback(port);
        cleanup_connection(port);
    }

    // timeout
    if (uip_timedout())
    {
        if (__uip_ore_config.timeout_callback != NULL)
            __uip_ore_config.timeout_callback(port);
        cleanup_connection(port);
    }
    
}

void uip_ore_initialize(uip_ore_config *c)
{
    struct in_addr my_in;
    unsigned short ip[2];
    int ret; 

    // extract config
    __uip_ore_config = *c;

    // init uip and the oredev
    uip_init();
    oredev_init();

#ifdef SENSOR
    ret = ferret_create(101, 0, 0, FERRET_SCALAR,
                        0, "0:1000000:/", sc, &malloc);
    if (ret)
        LOG_printf("Could not create sensor: %d\n", ret);
#endif
    // setup IP address
    ret = inet_aton(__uip_ore_config.ip, &my_in);
    if (ret)
    {
        printf("setting IP address to %s\n", __uip_ore_config.ip);
        memcpy(ip, &my_in.s_addr, 4);
        uip_sethostaddr(ip);
    }
    else
    {
        printf("Error parsing IP address %s\n", __uip_ore_config.ip);
    }

    // setup port to listen
    if (__uip_ore_config.port_nr == 0)
    {
        printf("NOT listening to any port.\n");
    }
    else
    {
        printf("listening to port nr. %hu (%hu)\n", __uip_ore_config.port_nr, HTONS(__uip_ore_config.port_nr));
        uip_listen(HTONS(__uip_ore_config.port_nr));
        if (add_port(__uip_ore_config.port_nr) < 0)
        {
            LOG_Error("Could not add port entry.");
            exit(1);
        }
    }
}

/* uIP thread func */
void uip_ore_thread(void *arg)
{
    short arptimer=0;

    // startup notification
    l4thread_started(NULL);

    printf("entering thread loop.\n");
    // thread loop
    while (1)
    {
        // try to read a packet - this will sleep at most
        // 0.5 seconds and then return.
        uip_len = oredev_read();

#ifdef SENSOR
        ferret_scalar_put(sc, sc_cnt++);
#endif

        // no data received?
        if (uip_len == 0)
        {
            int i;
        
            // Poll the open connections - this enables the 
            // connections to send data through the callback
            // function. For fairness' sake we poll round-robbin.
            for (i=0; i < UIP_CONNS; i++)
            {
                uip_periodic(i);
                // send data if the connection scheduled some
                if (uip_len > 0)
                {
                    uip_arp_out();
                    oredev_send();
                }
            }
        }
        else // read some data
        {
            // IP packet
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
            // ARP packet
            else if (BUF->type == htons(UIP_ETHTYPE_ARP))
            {
                uip_arp_arpin();
                if (uip_len > 0)
                {
                    oredev_send();
                }
            }
        }

        // increment the ARP timer every 10 seconds
        if (++arptimer == 100)
        {
            uip_arp_timer();
            arptimer = 0;
        }
    }
}
