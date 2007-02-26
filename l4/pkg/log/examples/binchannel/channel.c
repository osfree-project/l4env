/*! 
 * \file   log/examples/binchannel/channel.c
 * \brief  Example for binary channel use.
 *
 * \date   10/05/2001
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */

#include <l4/log/l4log.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/types.h>
#include <stdio.h>
#include <string.h>

char LOG_tag[9]="channel";
unsigned char buffer[2*L4_PAGESIZE];

int main(void){
    unsigned ptr, i;
    int err, id, len;

    LOG("Hi, here we are. We open the channel now.");

    memset(buffer, 0, sizeof(buffer));

    /* obtain a page pointer */
    ptr = ((l4_umword_t)(buffer+L4_PAGESIZE-1))&L4_PAGEMASK;
    id = LOG_channel_open(2, l4_fpage(ptr, L4_LOG2_PAGESIZE, 0, 0));

    LOG("opening channel returned %d", id);
    if(id<0) return 1;

    i=0;
    do{
	sprintf((char*)ptr, "This is our string nr %d\n", i);
	i++;
	len = strlen((char*)ptr)+1;
	err = LOG_channel_write(id, 0, len);
	LOG("Writing %d bytes to the channel returned %d", len, err);
	if(err) break;

	LOG("Flushing now.");
	err = LOG_channel_flush(id);
	LOG("Flushing returned %d. Closing channel.", err);
	if(err) break;
    } while(1);
    err = LOG_channel_close(id);
    LOG("Closing returned %d. Exiting...", err);
    return 0;
}
