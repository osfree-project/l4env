/**
 * inftpm_io.c - io register access functions
 *
 * A device driver for a Infineon TPM (SLD9630TT)
 *
 * Copyright (C) 2004 Bernhard Kauer <kauer@os.inf.tu-dresden.de>
 */

#include <linux/init.h>
#include <linux/pci.h>

#include "inftpm.h"
#include "inftpm_io.h"

enum TPM_IO_REGISTERS  
{
	// TPM write register to FIFO
	TPM_IO_WRFIFO  = 0,
	// TPM read register for FIFO
	TPM_IO_RDFIFO,
	// TPM status register
	TPM_IO_STAT,
	// TPM command register
	TPM_IO_CMD
};

enum TPM_IO_CMD_BITS
{
	// interrupt disable
	TPM_IO_CMD_BIT_DIS       =  0,
        // close loop for LPC
	TPM_IO_CMD_BIT_LP,
        // reset for TPM
	TPM_IO_CMD_BIT_RES,
        // Clear IRQ
	TPM_IO_CMD_BIT_IRQC      = 6,
};

enum TPM_IO_STAT_BITS
{
	// transmit FIFO empty
	TPM_IO_STAT_BIT_XFE = 0,
	// indication for LPC loop
	TPM_IO_STAT_BIT_LPA,
	// firmware ok?
	TPM_IO_STAT_BIT_FOK,
	// Selftest ok?
	TPM_IO_STAT_BIT_TOK,
        // IRQ active
	TPM_IO_STAT_BIT_IRQA = 6,
	// receive data available
	TPM_IO_STAT_BIT_RDA,
};

/**
 * Set a bit in the cmd io register
 */
static inline int
tpm_io_cmd_set(u16 base_addr, u8 bit, char set)
{

	char tmp1;
	tmp1=inb(base_addr+TPM_IO_CMD);
	if (set)
		tmp1 |= 1 << bit;
	else
		tmp1 &= ~(1<<bit);
	outb(tmp1,base_addr+TPM_IO_CMD);	
	return 0;
}

/**
 * Read the io status register.
 */
//static inline
u8
tpm_io_get_status(short base_addr)
{
	return inb(base_addr + TPM_IO_STAT);
}

/**
 * Waits until some flags are on or a timeout occurred.
 */
static inline int
tpm_io_wait_ready(u16 base_addr, u8 bit, u8 value)
{
	int timeout=0;
	int status;
	do 
	{
		status = tpm_io_get_status(base_addr);		
		if (((status & (1<<bit))>>bit) == value)
			break;
		else
		{		  
			timeout++;
			// every 10 times we wait some time
			if ((timeout>=10) && (timeout%10==0))
			{
				current->state = TASK_UNINTERRUPTIBLE;
				schedule_timeout((timeout/10)-1);
			}
		}
	} while(timeout < 1000);
	if (timeout >= 1000)
		error("timeout for bit %d at status 0x%02x",bit,status);
	return timeout < 1000 ? timeout : -ETIMEDOUT;
}


/**
 * Read a byte from the rdfifo. Returns the byte or a value less than
 * 0 if no bytes are available.
 */
static inline int
tpm_io_read_fifo(u16 base_addr){
	int res = -EIO;
	if (tpm_io_get_status(base_addr) & (1<<TPM_IO_STAT_BIT_RDA))
		res=inb(base_addr + TPM_IO_RDFIFO);
	else
		error("RDA not set");
	return res;
}

/**
 * Write a byte to the wrfifo. Returns a value less than 0 if the byte
 * could not be written.
 */
static inline int
tpm_io_write_fifo(u16 base_addr,u8 value){
	int res=0;
	if (tpm_io_get_status(base_addr) & (1<<TPM_IO_STAT_BIT_XFE))
		outb(value,base_addr + TPM_IO_WRFIFO);
	else
	{
		error("XFE not set: %02x",res);
		res= -EIO;
	}
	return res;
}


/**
 * Clear the read fifo of the tpm by reading the data.
 */
int
tpm_io_clear_fifo(u16 base_addr)
{
	int res;
	
	res = tpm_io_get_status(base_addr) & (1<<TPM_IO_STAT_BIT_RDA);

	if (res)
	{
		do
		{
			res = tpm_io_read_fifo(base_addr);
		} while (res>=0);
		res = 0;
	}
	else
		res=0;

	return res!=0;
}

/**
 * Wait for the firmware to be ready.
 */
static inline int 
tpm_io_wait_for_fw(short base_addr)
{
	int res;
	if ((res=tpm_io_wait_ready(base_addr,TPM_IO_STAT_BIT_FOK,0))<0)
		error("timeout waiting firmware");		
	return res;
}

/**
 * Reset the TPM.
 */
int
tpm_io_reset(u16 base_addr)
{
	int res;
	res=tpm_io_cmd_set(base_addr, TPM_IO_CMD_BIT_RES, 1);	
	return res;
}

/**
 * Init the status register of the tpm.
 */
int __init
tpm_io_init(u16 base_addr)
{
	int res=0;

	// disable loop bit
	if ((res=tpm_io_cmd_set(base_addr, TPM_IO_CMD_BIT_LP, 0)))
	  return res;

	// disable the interrupt
	if ((res=tpm_io_cmd_set(base_addr, TPM_IO_CMD_BIT_DIS, 1)))
	  return res;
 
	if ((res=tpm_io_wait_for_fw(base_addr)))
		return res;

	// clear fifo
	if ((res=tpm_io_clear_fifo(base_addr)))
		return res;

	return res;
}

/**
 * Wait until the tpm is ready for the next byte and send this byte.
 * Returns an errorcode or the time we waited.
 */
int
tpm_io_write(u16 base_addr, u8 value)
{
	int res;
	int res2;

	debug("> base %04x value %02x",base_addr,value);

	if ((res = tpm_io_wait_ready(base_addr, TPM_IO_STAT_BIT_XFE,1))<0)
		return res;
	res2=tpm_io_write_fifo(base_addr, value);
	res = res2 !=0 ? res2: res;
	
	debug("< = %d",res);
	return res;
}

/**
 * Wait until the tpm has a byte ready and read this byte.
 */
int
tpm_io_read(u16 base_addr, u8 *value)
{
	int res;

	debug("> base %04x value %x",base_addr,value);
	if ((res = tpm_io_wait_ready(base_addr, TPM_IO_STAT_BIT_RDA,1))<0)
		return res;
	res = tpm_io_read_fifo(base_addr);
	if (res>=0)
		*value = res & 0xff;
	debug("< = %d value %x",res,*value);
	return res;
}
