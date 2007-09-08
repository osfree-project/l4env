/**
 * inftpm_tl.c - transport layer
 *
 * A device driver for a Infineon TPM (SLD9630TT)
 *
 * Copyright (C) 2004 Bernhard Kauer <kauer@os.inf.tu-dresden.de>
 */

#include <linux/netdevice.h>
#include "inftpm.h"
#include "inftpm_io.h"
#include "inftpm_tl.h"

#define MAX_ERRORS       2

struct tpm_tl_header {
	u8 version;
	u8 ctrl;
	u16 size;
};

// default version info for LPC layer
#define TPM_DEFAULT_LPCVER   0x01
// default control byte for LPC layer
#define TPM_DEFAULT_CONTROL  0x00

// definition for TPM control byte
#define TPM_CTRL_CHAINING    0x80
#define TPM_CTRL_CHAININGACK 0x40
#define TPM_CTRL_ERROR       0x20
#define TPM_CTRL_WTX         0x10
#define TPM_CTRL_ABORT       0x08
#define TPM_CTRL_DATA        0x04

/**
 * Receive a block from the tpm in the transport layer.
 *
 * Returns:
 *  the size received
 *  -EAGAIN on NAK error
 *  -EBUSY  on WTX request
 *  -ETIMEOUT on a timeout
 *  -EINVAL on other errors
 */
static int
tpm_tl_recv(u16 base_addr, u8 *buf, size_t size)
{
	int count=0;
	int res=0;
	u8 tmp;
	struct tpm_tl_header header = {0,0,0};

	debug("> base %04x buf %x size %d",base_addr,buf,size);

	if (!(buf && size))
		return -EINVAL;

	// recv header
	for (count=0;(res>=0)&&count<sizeof(header);count++)
		res=tpm_io_read(base_addr, ((u8 *)&header)+count);

	header.size=ntohs(header.size);
		
	if (res<0){
		debug("recv header %d",res);
		return res;
	}


	debug("tl header %02x %02x %04x",
	      header.version,
	      header.ctrl,
	      header.size);

	// limit size
	size=size > header.size ? header.size : size;

	if (header.version != TPM_DEFAULT_LPCVER)
	{
		debug("lpc version mismatch %02x vs %02x at %d res %02x",
		      header.version,TPM_DEFAULT_LPCVER,count,res);
		return -EINVAL;
	}

	if (header.ctrl == TPM_CTRL_DATA)
	{
		if (header.size <= size)
		{		
			debug("recv data size: %d",size);
			for (count = 0;(res >= 0) && (count < size); count++)
				res=tpm_io_read(base_addr, &(buf[count]));
			if (res >= 0)
				res=header.size;
			debug("recv data = %d count %d",res,count);
		}
		else
		{
			printk(KERN_ERR MODULE_NAME "buffer to small");
			return -EINVAL;

		}
	}
	else if (header.ctrl & TPM_CTRL_ERROR)
	{
		res=tpm_io_read(base_addr, &tmp);
		if (res==0x15)
		{
			debug("send_error");
			res = - EAGAIN;
		}
		else
			debug("error: %02x",res);
			
	}
	else if (header.ctrl & TPM_CTRL_WTX)
		res = -EBUSY;
	else 
		res = -EINVAL;

	// XXX is this usefull here?
	// tpm_io_clear_fifo(base_addr);
	debug("< = %d",res);
	return res;
}

/**
 * Send a block to the tpm in the transport layer.
 */
static int
tpm_tl_send(u16 base_addr, u8 ctrl, u8 *buf, size_t size)
{
	int count = 0;
	int res = 0;
	struct tpm_tl_header header = {TPM_DEFAULT_LPCVER, ctrl, htons(size)};
	
	debug("> base %04x ctrl %02x buf %x size %d",base_addr,ctrl,buf,size);

	if ((!buf) && size != 0)
		return -EINVAL;
	
	// send header
	for (count = 0; (res>=0) && count < sizeof(header); count++)
		res = tpm_io_write(base_addr, ((u8 *)&header)[count]);

 	// send data
	for (count = 0; (res>=0) && count < size; count++)
		res = tpm_io_write(base_addr, buf[count]);

	debug("< =%d count %d",res,count);
	return res;
}


/**
 * Transmit a buffer to the tpm and wait for the response.
 *
 * The response is written to the buffer.
 *
 * Returns the number of bytes received or an error code.
 */
int
tpm_tl_transmit(u16 base_addr, u8 *buf, size_t send_size, size_t recv_size)
{
	int res;
	int error = 0;
	enum {
		TX_DATA,
		RX_DATA,
		TX_WTX,
		TX_NAK,		
	} state = TX_DATA;

	debug("> base %04x buf %x ssize %d rsize %d",
	     base_addr,
	     buf,
	     send_size,
	     recv_size);

	do
	{
		switch(state)
	        {
		case TX_DATA:
			debug("TX_DATA");
			if ((res = tpm_tl_send(base_addr, TPM_CTRL_DATA,
					       buf, send_size))<0)
				return res;
			else
				state=RX_DATA;
			break;
		case RX_DATA:
			debug("RX_DATA");			
			res = tpm_tl_recv(base_addr, buf, recv_size);
			if (res>=0)
				return res;
			else if (res == -EAGAIN)
			{
				error++;				
				if (error>MAX_ERRORS)
				{
					error("to much errors while RX_DATA");
					return -EIO;
				}
				else state = TX_DATA;
			}
			else if(res == -EBUSY) state=TX_WTX;
			else if(res == -EINVAL)
			{
				error++;
				if (error>MAX_ERRORS)
				{
					error("wait to long for tpm");
					return -EIO;
				}
				else state = TX_NAK;
			}
			else 
				return res;
			break;

		case TX_WTX:
			debug("TX_WTX");
			res = tpm_tl_send(base_addr, TPM_CTRL_WTX, 0, 0);
			if (res>=0)
				state=RX_DATA;
			else
				return res;
			break;

		case TX_NAK:
			debug("TX_NAK");
			res = tpm_tl_send(base_addr, TPM_CTRL_ERROR, 0, 0);
			if (res>=0)
				state=RX_DATA;
			else
				return res;
			break;
		}
	} while(1);
}

