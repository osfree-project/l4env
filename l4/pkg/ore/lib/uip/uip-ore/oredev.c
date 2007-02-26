/* ORe driver for the UIP TCP/IP stack.
 *
 * Bjoern Doebel <doebel@os.inf.tu-dresden.de>
 * 2005-12-12
 */

/*
 * Copyright (c) 2001, Swedish Institute of Computer Science.
 * All rights reserved. 
 *
 * Redistribution and use in source and binary forms, with or without 
 * modification, are permitted provided that the following conditions 
 * are met: 
 *
 * 1. Redistributions of source code must retain the above copyright 
 *    notice, this list of conditions and the following disclaimer. 
 *
 * 2. Redistributions in binary form must reproduce the above copyright 
 *    notice, this list of conditions and the following disclaimer in the 
 *    documentation and/or other materials provided with the distribution. 
 *
 * 3. Neither the name of the Institute nor the names of its contributors 
 *    may be used to endorse or promote products derived from this software 
 *    without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND 
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE 
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL 
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS 
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) 
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT 
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY 
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF 
 * SUCH DAMAGE. 
 *
 * Author: Adam Dunkels <adam@sics.se>
 */
#include <l4/ore/ore.h> 
#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/sys/ipc.h>

#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/socket.h>

#include <oredev.h>
#include "uip.h"
#include "uip_arp.h"
#include "uipopt.h"

static int ore_handle;
static l4ore_config ore_conf;
static unsigned char mac[6];
static l4_timeout_t recv_to;

static char *rx_buf;

#define CONFIG_OREDEV_TIMEOUT   500000

/*-----------------------------------------------------------------------------------*/
void
oredev_init(void)
{
    int mant = 0;
    int exp  = 0;    
    struct uip_eth_addr my_mac;
    
    ore_conf   = L4ORE_DEFAULT_CONFIG;
    ore_conf.rw_broadcast = 1;
    ore_handle = l4ore_open("eth0", mac, &ore_conf);
    if (ore_handle < 0)
    {
        char buf[100];
        LOG("Could not open connection to ORe: %d", ore_handle);
        exit(1);
    }

    LOG_MAC(1, mac);
    
    // setup rx timeout
    l4util_micros2l4to(CONFIG_OREDEV_TIMEOUT, &mant, &exp);
    recv_to = L4_IPC_TIMEOUT(0,0,mant,exp,0,0);

    rx_buf = malloc(1520);
    
    // copy MAC
    memcpy(&my_mac.addr[0], &mac[0], 6); 
    uip_setethaddr(my_mac);
}
/*-----------------------------------------------------------------------------------*/
unsigned int
oredev_read()
{
  int ret, m, ex;
  int size = UIP_BUFSIZE;
 
  ret = l4ore_recv_blocking(ore_handle, &rx_buf, &size, recv_to);
  if(ret != 0) 
  {
     return 0;
  }

  memcpy(uip_buf, rx_buf, size);

  return size;
}
/*-----------------------------------------------------------------------------------*/
void
oredev_send(void)
{
    int ret;
    char buf[UIP_BUFSIZE];

    memcpy(buf, uip_buf, UIP_LLH_LEN + 40);
    if (uip_len > 40 + UIP_LLH_LEN)
    {
        memcpy(buf+UIP_LLH_LEN+40, (char *)uip_appdata, uip_len - 40 - UIP_LLH_LEN);
    }

    ret = l4ore_send(ore_handle, buf, uip_len);
    
    if (ret != 0)
    {
        LOG("Error on l4ore_send(): %d", ret);
    }
}  
/*-----------------------------------------------------------------------------------*/
