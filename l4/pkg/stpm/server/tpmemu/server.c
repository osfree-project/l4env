/*
 * \brief   
 * \date    2008-01-16
 * \author  Alexander Boettcher <boettcher@tudos.org>
 */
/*
 * Copyright (C) 2008  Alexander Boettcher <boettcher@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <stdio.h>
#include <stdlib.h>

#include "tpm/tpm_emulator.h"

#include <l4/names/libnames.h>
#include <l4/util/rdtsc.h>
#include <l4/util/util.h> //l4_sleep
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include "stpm-server.h"
#include "stpmif.h"

int l4libc_heapsize = 1024 * 1024;

void *CORBA_alloc(unsigned long size){
	return malloc(size);
}
void CORBA_free(void *addr){
	free(addr);
}

int
stpmif_transmit_component (CORBA_Object _dice_corba_obj,
                           const char *write_buf,
                           unsigned int write_count,
                           char **read_buf,
                           unsigned int *read_count,
                           CORBA_Server_Environment *_dice_corba_env)
{

  int res;

  if (write_buf == NULL || read_buf == NULL || read_count == NULL)
    return -L4_EINVAL;

  res = tpm_handle_command((const unsigned char *)write_buf, write_count,
                           (uint8_t **)read_buf, read_count);
  if (res < 0) {
    LOG_Error("tpm_handle_command failed.");
  }

  if (res > 0)
  {
    *read_count = 0;
    return res;
  }

  return *read_count;
}

CORBA_int
stpmif_abort_component(CORBA_Object _dice_corba_obj,
		       CORBA_Server_Environment *_dice_corba_env)
{
    return -L4_EINVAL;
}

int main(void)
{
  if (names_register("vtpmemu") == 0)
  {
    LOG_Error("Registration error at nameserver\n");
    return 1;
  }

  if (l4_calibrate_tsc() == 0)
  {
     LOG_Error("Error in time stamp counter calibration ...\n");
     return 1;
  }

  l4_sleep(2000);

  // clear 1
  // save 2
  // deactivated 3
  tpm_emulator_init(1);

  DICE_DECLARE_SERVER_ENV(env);
	env.malloc=CORBA_alloc;
	env.free=CORBA_free;

  stpmif_server_loop(&env);
}
