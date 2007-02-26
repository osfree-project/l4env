/* Example for the use of libuip_ore: This example shows sending 
 * and receiving data through the uIP stack using the uip_ore library.
 *
 * Test:
 *  1. Chose your IP and port and compile the example.
 *  2. at your computer: netcat -l -p <port>
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

#define MY_IP       "141.76.48.30"
#define MY_PORT     1234
#define REMOTE_IP   "141.76.48.34"
#define REMOTE_PORT 9000

#define BUF_SIZE    15000000
#define PACKET_SIZE 1250

char *the_message;

static unsigned __lport;
static unsigned __con = 0;
static int p_sent = 0;
static int p_acked = 0;

// callback functions
void receive(const void *buf, const unsigned size, unsigned port);
void ack(void *addr, unsigned port);
void rexmit(void *addr, unsigned size, unsigned port);
void connected(const struct in_addr ip, unsigned port);
void timeout(unsigned port);
void abrt(unsigned port);
void close(unsigned port);

void receive(const void *buf, const unsigned size, unsigned port)
{
    if (size == 0)
        return;
        
    LOG("\033[33mreceived %d bytes at %p\033 on port %u[0m", size, buf, port);
    // hah! if we want to print this message out, we need to
    // make it a 0-ending string, otherwise the whole rest of
    // the message buffer will be printed until a zero occurs.
    ((char *)buf)[size] = 0;
    // probably the last char of the message is still a \n...
    if (((char *)buf)[size-1] == '\n')
        ((char *)buf)[size-1] = 0;

    LOG("text = '%s'", (char *)buf);
}

void ack(void *addr, unsigned port)
{
    LOGd(VERBOSE, "ack'ed %p", addr);
    p_acked += 1;
}

void rexmit(void *addr, unsigned size, unsigned port)
{
    LOGd(VERBOSE, "retransmitting %p", addr);
    uip_ore_send(addr, size, REMOTE_PORT);
}

void connected(const struct in_addr ip, unsigned port)
{
//    char *buf = malloc(100);
//    strcpy(buf, "Hello, server!\n");
//    LOG("Connected to %s. Saying hello over port %hu", inet_ntoa(ip), port);
//    uip_ore_send(buf, strlen(buf), port);
    __lport = port;
    __con = 1;
}

void timeout(unsigned port)
{
    int ret;
    struct in_addr ip;
    
    LOG("connection timed out, retrying.");
    ret = inet_aton(REMOTE_IP, &ip);
    if (ret)
    {
        LOG("connecting to %s:%d", REMOTE_IP, REMOTE_PORT);
        uip_ore_connect(ip, REMOTE_PORT);
    }    
    __con = 0;
}

void abrt(unsigned port)
{
    LOG("connection aborted");
    __con = 0;
}

void close(unsigned port)
{
    LOG("connection on port %d closed.", port);
    __con = 0;

}

int main(void)
{
    uip_ore_config c;
    l4thread_t t;
    struct in_addr ip;
    int ret,i;
 
    the_message = malloc(BUF_SIZE);
    for (i = 0; i < BUF_SIZE; i++)
    {
        if (i % 10 == 0)
            the_message[i] = 0xEE;
        else
            the_message[i] = 0xAA;
    }
    
    // setup configuration
    memset(&c, 0, sizeof(uip_ore_config));
    strcpy(c.ip, MY_IP);
    c.port_nr           = 0;
    c.recv_callback     = receive;
    c.ack_callback      = ack;
    c.rexmit_callback   = rexmit;
    c.connect_callback  = connected;
    c.abort_callback    = abrt;
    c.timeout_callback  = timeout;
    c.close_callback    = close;

    uip_ore_initialize(&c);
    // create uip worker thread
    t = l4thread_create(uip_ore_thread, NULL, L4THREAD_CREATE_SYNC);

    // establish connection
    ret = inet_aton(REMOTE_IP, &ip);
    if (ret)
    {
        LOG("connecting to %s:%d", REMOTE_IP, REMOTE_PORT);
        uip_ore_connect(ip, REMOTE_PORT);
    }
    else
    {
       LOG("Could not parse IP address.");
       exit(1);
    } 

    // periodically send data 
    while(1)
    {
        if (__con && p_sent < BUF_SIZE / PACKET_SIZE)
        {
            uip_ore_send( (the_message + p_sent * PACKET_SIZE), PACKET_SIZE, __lport);
            p_sent++;
        }
        else
        {
            LOG("sent %d, acked %d", p_sent, p_acked);
            l4_sleep(2000);
        }
    }

    return 0;
}
