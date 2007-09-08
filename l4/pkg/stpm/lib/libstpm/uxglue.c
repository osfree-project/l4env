/*
 * \brief   Gluefile for using the host's TPM under Fiasco-UX.
 * \date    2004-09-15
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2004 Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/lxfuxlibc/lxfuxlc.h>
#include "stpmif.h"
#include "my_cfg.h"
#include <errno.h>

static char *TPM_DEV="/dev/tpm0";


/**
 * Transmit a blob to the TPM
 */
int stpm_transmit(const char *write_buf, unsigned int write_count,
                  char **read_buf, unsigned int *read_count)
{
  int fd;
  
  fd = lx_open(TPM_DEV, LX_O_RDWR, 0);

  if (fd >= 0)
    {
      int ret;

      if (0 < (ret = lx_write(fd, write_buf, write_count)))
    	ret = lx_read(fd, *read_buf, *read_count);

      if (0 < ret)
	*read_count = ret;
      else
        *read_count = 0;
		
      lx_close(fd);
      return ret;
    }
  else
    return -ENODEV;
}


/**
 * Abort the tpm call.
 */
int stpm_abort(void)
{
  return -ENODEV;
}
