/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

/* Example for the use of libuip_ore: This example shows sending 
 * and receiving data through the uIP stack using the 
 * uip_ore library.
 *
 * Test by running "nc 141.76.48.47 1234" from your workstation. (And
 * maybe adapt the IP and port configuration below, if you need to.)
 *
 * XXX: netcat/nc are EVIL! If transmitting high amounts of data with uIP,
 *      you should avoid these tools, because of some flaws in uIP's 
 *      TCP stack that make communication very slow.
 */
#include <stdlib.h>
#include <string.h>

#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/ore/ore.h>
#include <l4/ore/uip-ore.h>
#include <l4/thread/thread.h>

#include <arpa/inet.h>

#define VERBOSE 0
#define MY_IP   "141.76.48.30"
#define MY_PORT 1234

static const char *the_message = "Hello world\n";
static int closed = 0;

void receive(const void *buf, const unsigned size, unsigned port);
void ack(void *addr, unsigned port);
void rexmit(void *addr, unsigned size, unsigned port);
void connected(const struct in_addr ip, unsigned port);
void timeout(unsigned port);
void abrt(unsigned port);
void close(unsigned port);

void receive(const void *buf, const unsigned size, unsigned port)
{
    LOG("\033[33mreceived %d bytes at %p\033[0m", size, buf);
    // hah! if we want to print this message out, we need to
    // make it a 0-ending string, otherwise the whole rest of
    // the message buffer will be printed until a zero occurs.
    // FIXME: this violates the const-modifier for buf !!!
    ((char *)buf)[size] = 0;
    // probably the last char of the message is still a \n...
    if (((char *)buf)[size-1] == '\n')
        ((char *)buf)[size-1] = 0;

    LOG("text = '%s'", (char *)buf);
}

void ack(void *addr, unsigned port)
{
    LOGd(VERBOSE, "ack'ed %p", addr);
    // free, because addr has been malloc'ed earlier
    free(addr);
}

void rexmit(void *addr, unsigned size, unsigned port)
{
    LOGd(VERBOSE, "retransmitting %p", addr);
    uip_ore_send(addr, size, MY_PORT);
    
}

void connected(const struct in_addr ip, unsigned port)
{
    char *buf = malloc(100);
    strcpy(buf, "Hello, server!\n");
    LOG("Connected to %s. Saying hello.", inet_ntoa(ip));

    // we can even send new data in a callback, BUT ONLY ONCE!
    uip_ore_send(buf, strlen(buf), MY_PORT);
}

void timeout(unsigned port)
{
    LOG("connection timed out.");
    closed = 1;
}

void abrt(unsigned port)
{
    LOG("connection aborted");
    closed = 1;
}

void close(unsigned port)
{
    LOG("connectin closed\n");
    closed = 1;
}

int main(void)
{
    uip_ore_config c;
    l4thread_t t;
    
    // setup configuration
    memset(&c, 0, sizeof(uip_ore_config));
    strcpy(c.ip, MY_IP); 
    c.port_nr           = 1234;         // port
    c.recv_callback     = receive;
    c.ack_callback      = ack;
    c.rexmit_callback   = rexmit;
    c.connect_callback  = connected;
    c.abort_callback    = abrt;
    c.timeout_callback  = timeout;

    uip_ore_initialize(&c);
    // create uip worker thread
    t = l4thread_create(uip_ore_thread, NULL, L4THREAD_CREATE_SYNC);

    // periodically send data 
    while(1 && !closed)
    {
        // create a dummy copy with dynamic memory 
        // --> you do not need to do this!
        char *send_buf = malloc(strlen(the_message)+1);
        strcpy(send_buf, the_message);

        LOGd(VERBOSE, "sending %p, %d", send_buf, strlen(send_buf));
        uip_ore_send(send_buf, strlen(send_buf), MY_PORT);
        l4_sleep(1000);
    }

    LOG("done");
    return 0;
}
