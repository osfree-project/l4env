/****************************************************************
 * (c) 2005 - 2007 Technische Universitaet Dresden              *
 * This file is part of DROPS, which is distributed under the   *
 * terms of the GNU General Public License 2. Please see the    *
 * COPYING file for details.                                    *
 ****************************************************************/

/*
 * \brief   Adaption of mini_http_uip providing self-healing
 * \date    2006-04-07
 * \author  Bjoern Doebel <doebel@os.inf.tu-dresden.de>
 */

/* Info:
 *
 * This adaption of the mini_http_uip web server adds monitoring
 * capabilities to this application. It is able to read the Ferret
 * sensor inside the uIP stack library which is providing a heartbeat.
 * 
 * By periodically checking this heartbeat, the monitor can detect, if
 * the worker thread is hanging (e.g., because of a temporary resource
 * leak or just because some stupid idiot implemented this server to
 * hang after every 4 http requests (see the ack_callback function!)).
 * 
 * Upon such a hangup, the monitor simply restarts the http worker thread.
 * Remote clients only experience a delay in execution of their request,
 * but they do not see non-recoverable failures. Furthermore the http
 * server state is preserverd throughout restarts.
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
#include <l4/thread/thread.h>
#include <l4/log/l4log.h>
#include <l4/util/macros.h>
#include <l4/util/parse_cmd.h>
#include <l4/ore/uip-ore.h>

#include <l4/ferret/monitor.h>
#include <l4/ferret/types.h>
#include <l4/ferret/util.h>
#include <l4/ferret/sensors/scalar_consumer.h>

/*** LOCAL INCLUDES ***/
#include "http.h"

#define BUF ((struct uip_eth_hdr *)&uip_buf[0])
#define BUFSIZE 1024

#define ISO_G        0x47
#define ISO_E        0x45
#define ISO_T        0x54

#define FERRET_HTTP_MAJOR   110
#define FERRET_HTTP_MINOR     0

char LOG_tag[9] = "httpheal";

static char sendbuf[BUFSIZE];
static struct http_state state;
static int visitor;
static char *ip_addr;
static l4thread_t worker;

char *httpmsg = 
	"HTTP/1.1 200 OK\n"
	"Date: Fri, 07 Apr 2006 11:11:32 CET\n"
	"Server:  healing_http (L4Env) DROPS\n"
	"Connection: close\n"
	"Content-Type: text/html; charset=iso-8859-1\n"
	"\n"
	"<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">\n"
	"<HTML><HEAD>\n"
	"<TITLE>Self-Healing HTTP server</TITLE>\n"
	"</HEAD><BODY bgcolor=#778899>\n"
	"<H1>Welcome to the HTTP server NOT running on FLIPS</H1>\n"
	"Thanks for visiting our informative site.\n"
    "It has been brought to you by the healing_http server.\n"
	"You are visitor number %d.<P>\n"
	"</BODY></HTML>\n";
    
void receive_cb(const void *buf, const unsigned size, unsigned port)
{
    if (port == 80)
    {
        // check if we are in HTTP_READY _and_ the packet
        // is a HTTP GET request
        if ((state.state == HTTP_READY)    &&
            (((char *)buf)[0]  == ISO_G)      &&
            (((char *)buf)[1]  == ISO_E)      &&
            (((char *)buf)[2]  == ISO_T) )
        {
            // print data into send buffer
            snprintf(sendbuf, BUFSIZE, httpmsg, ++visitor);
            LOG("visitor %d", visitor);
            // send()
            uip_ore_send(sendbuf, BUFSIZE, 80);
            state.state    = HTTP_GET;
        }
    }
}

void connect_cb(const struct in_addr ip, unsigned port)
{
    state.state = HTTP_READY;
    state.poll_count = 0;
    LOG("connection.");
}

static int bla = 0;
void ack_cb(void *addr, unsigned port)
{
    if (bla++ >= 3)
        while(1);

    uip_ore_close(80);
    state.state = HTTP_READY;
}

// initialize the HTTP server
void http_init(void)
{
    uip_ore_config conf;

    memset(&conf, 0, sizeof(uip_ore_config));
    strcpy(conf.ip, ip_addr);
    conf.port_nr = 80;
    conf.recv_callback = receive_cb;
    conf.ack_callback  = ack_cb;
    conf.connect_callback = connect_cb;

    uip_ore_initialize(&conf);

    worker = l4thread_create(uip_ore_thread, NULL, L4THREAD_CREATE_SYNC);

    visitor = 0;
}

void failure_restart(void)
{
    bla  = 0;
    l4thread_shutdown(worker);
    l4thread_create(uip_ore_thread, NULL, L4THREAD_CREATE_SYNC);
    LOG("\033[32mrestarted worker: "l4util_idfmt"\033[0m", l4util_idstr(l4thread_l4_id(worker)));
}

void monitor_self(void)
{
    int ret, new = -1, old = -1;
    ferret_scalar_t *sc;

    ret = ferret_att(101, 0, 0, sc);
    LOG("monitoring myself.");

    while (1)
    {
        old = new;
        new = ferret_scalar_get(sc);
        
        if (old == new)
        {
            LOG("\033[31mNo advance!?\033[0m");
            failure_restart();
        }

        l4_sleep(2000);
    }
}
    
// main routine
int main(int argc, const char **argv) {
    int ret;

    ret = parse_cmdline(&argc, &argv,
            'i', "ip", "specify IP address to use",
            PARSE_CMD_STRING, "127.0.0.1", &ip_addr,
            0);
    
    names_register("http");
    // run the worker
    http_init();
    
	LOG("HTTP server is up and accepting...");

    monitor_self();

    while (1);

	return 0;
}

