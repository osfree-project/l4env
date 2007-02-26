/**
 * \file	generic_fprov/examples/hostfs/main.c
 *
 * \date	07/28/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * File provider using the drivers in OSKit to access host file system.
 * We allow for one partition of one disc currently. Feel free to extend.
 */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/consts.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/generic_fprov/generic_fprov-server.h>
#include <l4/l4rm/l4rm.h>
#include <l4/util/getopt.h>
#include <l4/generic_io/libio.h>
#include <l4/util/parse_cmd.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "fs.h"

char LOG_tag[9]="hostfs";

/**
 * Return a new dataspace with the requested file in it
 * 
 * \param obj     	our client
 * \param name		requested file name
 * \param dm		dataspace manager for allocating the dataspace
 * \param flags		flags used for creating the dataspace
 * \retval ds		dataspace including the file image
 * \retial size		file size
 * \retval _ev		dice-generated environment
 *
 * \return 		- 0 on success
 * 			- -L4_ENOMEM    memory allocation failed
 *			- -L4_ENOTFOUND file not found
 *			- -L4_EIO       error reading file
 *			- -L4_EINVAL    error passing the dataspace to client
 */
l4_int32_t l4fprov_file_open_component(CORBA_Object obj,
				       const char* name,
				       const l4_threadid_t *dm,
				       l4_uint32_t flags,
				       l4dm_dataspace_t *ds,
				       l4_uint32_t *size,
				       CORBA_Server_Environment *env){
    int len;
    int err;
    char *addr;
    long long s;
    char buf[L4DM_DS_NAME_MAX_LEN];
    const char *ptr;
    fs_file_t *file;

    if ((file=fs_open(name, &s))==0) {
	/* file not found */
	LOG_Error("File \"%s\" not found", name);
	return -L4_ENOTFOUND;
    }

    *size = (l4_uint32_t)s;

    ptr = strrchr(name, '/');
    if(!ptr)
      ptr=name;
    else
      ptr++;
    snprintf(buf, L4DM_DS_NAME_MAX_LEN, "hostfs image: %s", ptr);

    if ((addr = l4dm_mem_ds_allocate_named(*size, flags, buf,
					    (l4dm_dataspace_t *)ds))==0) {
	LOG_Error("l4dm_mem_ds_allocate_named(size=%d)", *size);
	fs_close(file);
	return -L4_ENOMEM;
    }

    printf("Loading %s [%dkB]\n", name, (*size + 1023) / 1024);
  
    len = fs_read(file, 0, (char*) addr, *size);
    fs_close(file);

    /* detach dataspace */
    if ((err = l4rm_detach((void *)addr))) {
	LOG_Error("l4rm_detach(): %s", l4env_errstr(err));
	l4dm_close((l4dm_dataspace_t*)ds);
	return -L4_ENOMEM;
    }
  
    if (len==0){
	LOG_Error("reading file \"%s\".", name);
	l4dm_close((l4dm_dataspace_t*)ds);
	return -L4_EIO;
    }

    /* pass dataspace to our client */
    if ((err = l4dm_transfer((l4dm_dataspace_t *)ds,*obj))!=0) {
	LOG_Error("l4dm_transfer(): %s", l4env_errstr(err));
	l4dm_close((l4dm_dataspace_t*)ds);
	return -L4_EINVAL;
    }

    /* done */
    return 0;
}

int main (int argc, const char **argv){
    char *diskname;
    char *partname;

    if((parse_cmdline(&argc, &argv,
		      'd',"disk","disk name",
		      PARSE_CMD_STRING, "hda", &diskname,
		      'p',"part","partition name",
		      PARSE_CMD_STRING, "a", &partname,
		      0))!=0) exit(1);

    if (fs_init(diskname, partname)){
	LOG_Error("fs_init()");
	exit(1);
    }
    if (!names_register("hostfs")) {
	LOG_Error("failed to register at names");
	exit(1);
    }

    l4fprov_file_server_loop(NULL);
    return 0;
}

