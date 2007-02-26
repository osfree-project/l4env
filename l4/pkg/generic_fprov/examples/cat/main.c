/*!
 * \file	main.c
 * \brief	Simple cat program for file provider
 *
 * \date	06/03/2003
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author	Jork Loeser <jork@os.inf.tu-dresden.de>
 */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/l4rm/l4rm.h>
#include <l4/util/util.h>
#include <l4/env/env.h>
#include <l4/names/libnames.h>
#include <l4/generic_fprov/generic_fprov-client.h>
#include <l4/util/parse_cmd.h>

#include <stdio.h>
#include <stdlib.h>

char LOG_tag[9]="cat";

int main(int argc, const char **argv) {
    int err, ret=1;
    l4_threadid_t server_id;
    l4_threadid_t dm_id;
    l4dm_dataspace_t ds;
    void* addr;
    l4_size_t size;
    int l, off=0;
    CORBA_Environment _env = dice_default_environment;
    const char *servername;
    const char *filename;
  
    if((parse_cmdline(&argc, &argv,
		      's',"server","file provider name",
		      PARSE_CMD_STRING, "hostfs", &servername,
		      'n',"name","file name",
		      PARSE_CMD_STRING, "/etc/passwd", &filename,
		      0))!=0) exit(1);


    if (!names_waitfor_name(servername, &server_id, 10000)) {
	printf("Server \"%s\" not found\n", servername);
	return 1;
    }

    dm_id = l4env_get_default_dsm();
    if (l4_is_invalid_id(dm_id)){
	printf("No dataspace manager found\n");
	return 1;
    }

    if ((err = l4fprov_file_open_call(&server_id,
				      filename,
				      &dm_id,
				      0, &ds, &size, &_env))!=0){
	printf("Opening file \"%s\" at %s: %s\n",
	       filename, servername, l4env_errstr(err));
	return 1;
    }
  
    if ((err = l4rm_attach(&ds, size, 0,
			   L4DM_RO | L4RM_MAP, &addr))!=0){
	printf("Error attaching dataspace: %s\n", l4env_errstr(err));
	goto e_destroy;
    }

    printf("File %s opened at %p.\n", filename, addr);

    printf("---------------- Contents: -----------\n");
    while(size>0){
	l = size; if(l>75)l=75;
	printf("%.*s",75,(char*)addr+off);
	off+=l;
	size-=l;
    }
    printf("\n------------------- all -------------\n");

    /* we are set */
    ret=0;
    if((err = l4rm_detach(addr))!=0){
	LOG_Error("l4rm_detach(): %s", l4env_errstr(err));
    }

  e_destroy:
    if((err = l4dm_close(&ds))!=0){
	LOG_Error("l4dm_close(): %s", l4env_errstr(err));
    }
    return ret;
}
