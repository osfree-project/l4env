/*
 * \brief   Gluefile for the integration of tpm.c with dde_linux.
 * \date    2004-06-02
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2004  Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdlib.h>
#include <l4/crtx/ctor.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/util/macros.h>
#include <l4/util/rdtsc.h>
#include <l4/generic_io/libio.h>
#include <l4/dde_linux/dde.h>
#include <l4/dde_linux/ctor.h>
#include <l4/log/l4log.h>


/* Linux */
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/soundcard.h>
#include <linux/miscdevice.h>

#include <linux/pci.h>
#include "stpmif.h"
#include "my_cfg.h"

// we need this for the reading and writing
struct file_operations *fops;

/**
 * Set the fops from the given device.
 */
int misc_register(struct miscdevice *tpm_dev){
  LOG_printf("misc_register()\n");
  Assert(!fops);
  if (fops)
  {
        LOG_printf("registering driver twice - perhaps used more than one driver!\n");
	return -ENODEV;
  }
  fops=tpm_dev->fops;
  return 0;
}

/**
 * Remove the fops.
 */
int misc_deregister(struct miscdevice *tpm_dev){
  LOG_printf("misc_deregister()\n");
  fops=0;
  return 0;  
}

/**
 * A dummy function;
 */
loff_t no_llseek(struct file *file, loff_t offset, int origin){
  return -1;
}

///////////////////////////////////////////////////


/**
 * Transmit a buffer to the TPM.
 */
int stpm_transmit(const char *write_buf, unsigned int write_count, char **read_buf, unsigned int *read_count)
{
  int ret;

  if (fops)
    {
      if (0 < (ret = fops->write(0, write_buf, write_count, 0)))
	ret = fops->read(0, *read_buf, *read_count, 0);
      if (0 < ret)
        *read_count = ret;
      else
        *read_count = 0;
      return ret;
    }
  else
    return -ENODEV;
}

/**
 * Abort the tpm call.
 *
 * XXX Not implemented.
 */
int stpm_abort(void)
{
    return -ENODEV;
    
  //return fops->ioctl(0,0,TPMIOC_CANCEL,0);
}
///////////////////////////////////////////////////

static inline int
Error(char *fmt,int param)
{
  LOG_printf("Error: ");
  LOG_printf(fmt,param);
  LOG_printf("\n");
  return 0;
}

/**
 * Init everthing we need to do our job!
 */
static void
init_dde(void)
{
  l4io_info_t *io_info_addr = NULL;
  int err;
 

  l4_calibrate_tsc();
  
   /* request io info page for "jiffies" and "HZ" */
  l4io_init(&io_info_addr, L4IO_DRV_INVALID);
  
  /* initialize all DDE modules required ... */
  if ( (err = l4dde_mm_init(VMEM_SIZE, KMEM_SIZE) ) ) {
    Error("initializing mm (%d)", err);
    exit(-1);
  }  

  if ( ( err = l4dde_process_init() ) ) {
    Error("initializing process-level (%d)", err);
    exit(-1);
  }

  // we need time for schedule...
  if ( ( err = l4dde_time_init() ) ) {
    Error("initializing time (%d)", err);
    exit(-1);
  }

  // we have an pci driver
  if ((err = l4dde_pci_init())) {
    Error("initializing pci (%d)", err);
    exit(-1);
  }

  /* and initialize our drivers.*/
  l4dde_do_initcalls();
}

L4C_CTOR(init_dde, L4CTOR_AFTER_BACKEND);
