/*
 * \brief   STPM server.
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

#include <stdio.h>
#include <stdlib.h>
#include <l4/names/libnames.h>
#include "stpm-server.h"
#include "stpmif.h"


int l4libc_heapsize = 32*1024;

/**
 * Allocation functions for read()
 */
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

  return stpm_transmit(write_buf,write_count, read_buf, read_count);
}


CORBA_int
stpmif_abort_component(CORBA_Object _dice_corba_obj,
		       CORBA_Server_Environment *_dice_corba_env)
{
	return stpm_abort();
}



int main(void)
{
	CORBA_Server_Environment env=dice_default_server_environment;

	if (names_register(stpmif_name) == 0)
	{
		printf("Error registering at nameserver\n");
		return 1;
	}

	env.malloc=CORBA_alloc;
	env.free=CORBA_free;
	stpmif_server_loop(&env);
}
