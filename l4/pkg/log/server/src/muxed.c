/*!
 * \file   log/server/src/muxed.c
 * \brief  Functions dealing with opening/writing/flushing the binary data
 *
 * \date   10/05/2001
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/sys/types.h> 
#include <l4/sys/syscalls.h>
#include <l4/env/errno.h>
#include "../include/log_comm.h"
#include "stuff.h"
#include "muxed.h"
#include "flusher.h"

/*! The area of channel buffers. Each buffer is up to 2MB in size.
 *
 * The address of a specific buffer is calculated from the base address plus
 * the connection ID.
 * 
 */
static void	*channel_buffer_area = (void*)0x70000000;


/*!\brief Obtain the next free virtual buffer
 *
 * This function returns a flexpage containing a hole where buffer
 * from clients can be mapped in. This function always returns a
 * valid buffer, because the last one is never actually used.
 */
l4_fpage_t channel_get_next_fpage(void){
    int i;

    for(i=0; i<MAX_BIN_CONNS; i++){
	if(bin_conns[i].channel==0)
	    return l4_fpage((l4_umword_t)channel_buffer_area +
			    i*LOG_CHANNEL_BUFFER_SIZE,
			    LOG_LOG2_CHANNEL_BUFFER_SIZE, 0, 0);
    }
    return l4_fpage(0,0,0,0); // should never happen
}

/*!\brief Open a connection on a specific channel
 * 
 * \param message.d0/d1		the fpage containing the communication buffer
 * \param message.d2/d3		0
 * \param message.d4		the channel to use
 *
 * \retval >=0		a valid connection id. Use this id for
 *			further requests.
 * \retval -L4_ENOMAP	No more connections can be opened.
 */
int channel_open(void){
    int id;

    for(id=0; id<MAX_BIN_CONNS; id++){
	if(bin_conns[id].channel==0) break;
    }
    if(id==MAX_BIN_CONNS) return -L4_ENOMAP;

    // setup the connection data
    if (message.result.md.fpage_received == 0) {
      // no fpage mapped (eg kernel area)
      if ((message.d1 & L4_PAGEMASK) < 0xc0000000)
	return -L4_EINVAL;
      bin_conns[id].fpage.fpage = message.d1;
    } else {
      bin_conns[id].fpage = l4_fpage((l4_umword_t)channel_buffer_area +
				     id*LOG_CHANNEL_BUFFER_SIZE,
				     ((l4_fpage_t) message.d1).fp.size, 0, 0);
    }

    bin_conns[id].written = 0;
    bin_conns[id].flushed = 0;
    // and finally set the "open-flag" of the connection: the channel
    bin_conns[id].channel = message.d4;

    return id;
}

/*!\brief Write data on a specific channel
 *
 * \param message.d1		the channel id
 * \param message.d2		the offset in the area
 * \param message.d3		the length of the chunk to write
 *
 * \retval 0			success
 * \retval -L4_EINVAL		there was no open connection with the given id,
 *				or the offset/size was invalid
 * \retval -L4_EBUSY		there was still a write action in process
 */
int channel_write(void){
    unsigned int id = message.d1;

    if ( (id >= MAX_BIN_CONNS) || 
	 (bin_conns[id].channel == 0) ||
	 ((message.d2 + message.d3) > (1UL << bin_conns[id].fpage.fp.size)) )
      return -L4_EINVAL;

    if(bin_conns[id].written!=bin_conns[id].flushed) return -L4_EBUSY;

    bin_conns[id].addr = ((void*)(bin_conns[id].fpage.fpage & L4_PAGEMASK)) + message.d2;
    bin_conns[id].size = message.d3;
    bin_conns[id].written ++ ;
    return 0;
}

/*!\brief Flush a binary channel.
 *
 * \param message.d1		the channel id
 * 
 * \retval 0			success
 * \retval -L4_EINVAL		id was invalid
 *
 * This function does currently nothing spectacular. It just calls our
 * general-purpose flushing function, which flushes all buffers.
 */
int channel_flush(void){
    int id = message.d1;

    if(id>=MAX_BIN_CONNS || bin_conns[id].channel==0) return -L4_EINVAL;
    
    flush_buffer();
    return 0;
}


/*!\brief Close a specific channel
 *
 * \param message.d1		the channel id
 * 
 * \retval 0			success
 * \retval -L4_EINVAL		id was invalid
 *
 */
int channel_close(void){
    int id = message.d1;

    if(id>=MAX_BIN_CONNS || bin_conns[id].channel==0) return -L4_EINVAL;
    
    if(bin_conns[id].written!=bin_conns[id].flushed) flush_buffer();

    bin_conns[id].channel = 0;
    bin_conns[id].fpage.fpage = 0;
    l4_fpage_unmap(l4_fpage((l4_umword_t)channel_buffer_area+
			    id*LOG_CHANNEL_BUFFER_SIZE,
			    LOG_CHANNEL_BUFFER_SIZE, 0, 0), L4_FP_FLUSH_PAGE);
    return 0;
}


