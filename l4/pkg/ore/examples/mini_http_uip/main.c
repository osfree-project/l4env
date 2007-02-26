/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

/*
 * \brief   UIP test client, adapted from pkg/flips/examples/mini_http
 *          to use the UIP network stack
 * \date    2005-12-13
 * \author  Bjoern Doebel <doebel@os.inf.tu-dresden.de>
 */

/*** GENERAL INCLUDES ***/
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

/*** L4 INCLUDES ***/
#include <l4/names/libnames.h>
//#include <l4/thread/thread.h>
#include <l4/log/l4log.h>
#include <l4/util/parse_cmd.h>

#include "http.h"
#include "uip.h"
#include "uip_arp.h"

/*** LOCAL INCLUDES ***/

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])
#define BUFSIZE 1024

#define ISO_G        0x47
#define ISO_E        0x45
#define ISO_T        0x54

//static l4_threadid_t srv;
char LOG_tag[9] = "httpserv";

static char sendbuf[BUFSIZE];
static struct http_state *state;
static int visitor;
static char *ip_addr;

char *httpmsg = 
	"HTTP/1.1 200 OK\n"
	"Date: Wed, 14 Dec 2005 14:51:32 CET\n"
	"Server: mini_http_uip (L4Env) DROPS\n"
	"Connection: close\n"
	"Content-Type: text/html; charset=iso-8859-1\n"
	"\n"
	"<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
	"<HTML><HEAD>\n"
	"<TITLE>FLIPS Mini HTTP server</TITLE>\n"
	"</HEAD><BODY bgcolor=#778899>\n"
	"<H1>Welcome to the HTTP server NOT running on FLIPS</H1>\n"
	"Thanks for visiting our informative site.\n"
    "It has been brought to you by the mini_http_uip server.\n"
	"You are visitor number %d.<P>\n"
	"</BODY></HTML>\n";
    
// http callback - this routine is called by uIP every time a
// TCP/IP event occurs (packet arrives, acknowledgment, ...)
void http_appcall(void)
{
    // we only do something, if this connection is at port 80
    if (uip_conn->lport == HTONS(80))
    {
        // get http state from connection
        state = (struct http_state *)(uip_conn->appstate);

        // new connection opened, init values
        if (uip_connected())
        {
            state->state        = HTTP_READY;
            state->poll_count   = 0;
        }
        // poll from uip lib - abort connection after 10 idle polls
        else if (uip_poll())
        {
            if (++state->poll_count >= 10)
            {
                LOG("aborting connection");
                uip_abort();
            }
            return;
        }
        // new request
        else if (uip_newdata())
        {
            // check if we are in HTTP_READY _and_ the packet
            // is a HTTP GET request
            if ((state->state == HTTP_READY)    &&
                (uip_appdata[0]  == ISO_G)      &&
                (uip_appdata[1]  == ISO_E)      &&
                (uip_appdata[2]  == ISO_T) )
            {
                // print data into send buffer
                snprintf(sendbuf, BUFSIZE, httpmsg, ++visitor);
                LOG("visitor %d", visitor);
                // send()
                uip_send(sendbuf, BUFSIZE);
                state->state    = HTTP_GET;
            }
        }
        // if there was an acknowledgement and we are in
        // HTTP_GET state, we close the connection (because
        // we only send one packet!) and are HTTP_READY again
        if (uip_acked() && state->state == HTTP_GET)
        {
            uip_close();
            state->state = HTTP_READY;
        }
    }
}

// initialize the HTTP server
void http_init(void)
{
    struct in_addr in;
    unsigned short ip[2];
    int ret;

    // init UIP and the ORe device driver
    uip_init();
    oredev_init();

    // set up the IP address
    ret = inet_aton(ip_addr, &in);
    if (ret != 0)
    {
        LOG("Setting my IP address to %s", ip_addr);
        memcpy(ip, &in.s_addr, 4);
        uip_sethostaddr(ip);
    }
    else
    {
        LOG("Error parsing IP '%s'!", ip_addr);
    }

    visitor = 0;
    
    uip_listen(HTONS(80));
}
    
// main routine
int main(int argc, const char **argv) {
    unsigned arptimer = 0;
    int ret;

    ret = parse_cmdline(&argc, &argv,
            'i', "ip", "specify IP address to use",
            PARSE_CMD_STRING, "127.0.0.1", &ip_addr,
            0);
    
    names_register("http");
    http_init();
    
	LOG("HTTP server is up and accepting...");

    while(1)
    {
        // read packet
        uip_len = oredev_read();
        // if len is zero, the read() timed out...
        if (uip_len == 0)
        {
            int i;
            // check for new data on all open connections
            for (i = 0; i < UIP_CONNS; i++)
            {
                uip_periodic(i);
                // new data to send?
                if (uip_len > 0)
                {
                    // build ethernet packet
                    uip_arp_out();
                    // send packet
                    oredev_send();
                }
            }
        }
        else // a new packet arrived
        {
            // IP packet?
            if (BUF->type == htons(UIP_ETHTYPE_IP))
            {
                // remove ethernet header
                uip_arp_ipin();
                // call UIP lib
                uip_input();
                // if uip_len > 0, someone wants to send
                // data in reply to the last received packet
                if (uip_len > 0)
                {
                    uip_arp_out();
                    oredev_send();
                }
            }
            // ARP packet=
            else if (BUF->type == htons(UIP_ETHTYPE_ARP))
            {
                // arp it!
                uip_arp_arpin();
                // reply necessary?
                if (uip_len > 0)
                {
                    // send!
                    oredev_send();
                }
            }
        }

        // the ORe driver timeouts after 0.5 seconds
        // --> 20 timer ticks are about 10 seconds
        // --> we tick the arp timer every 10 sec to time out arp entries
        if (++arptimer == 20)
        {
            uip_arp_timer();
            arptimer = 0;
        }
    }

    // should never get here.
    LOG("going to sleep");
    while(1);

	return 0;
}

