/*
 * \brief   IDL client stubs of STPM interface.
 * \date    2004-11-12
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

#include <l4/stpm/stpm-client.h>
#include <l4/stpm/stpmif.h>
#include <l4/stpm/encap.h>

#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/sys/consts.h>

static l4_threadid_t server_id = L4_INVALID_ID;       
				
int stpm_transmit ( const char *write_buf, unsigned int write_count,
                    char **read_buffer, unsigned int *read_count)
{
  DICE_DECLARE_ENV(env);

  if (stpm_check_server(stpmif_name, 0)) return -L4_EINVAL;	

  return stpmif_transmit_call(&server_id, write_buf, write_count,
			      read_buffer, read_count, &env);
}

int stpm_abort (void)
{
  DICE_DECLARE_ENV(env);

  if (stpm_check_server(stpmif_name, 0)) return -L4_EINVAL;	

  return stpmif_abort_call(&server_id, &env);
}

int stpm_shutdown_on_exit (l4_taskid_t const * const task)
{
  DICE_DECLARE_ENV(env);

  return stpmif_shutdown_on_exitevent_of_call(&server_id, task, &env);
}

int stpm_check_server (const char * name, int recheck)
{							
  if (l4_is_invalid_id(server_id) || recheck)
    {							
      if (name == 0 || (name != 0 && strlen(name) == 0))
        return 2;

      if (!names_waitfor_name(name, &server_id, 10000))	
        return 1;					
    }							
  return 0;						
}

