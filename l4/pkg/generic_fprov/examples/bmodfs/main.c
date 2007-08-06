/* $Id$ */
/**
 * \file	generic_fprov/examples/bmodfs/main.c
 * \brief	File provider offering boot modules.
 *
 * \date	07/2004
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <stdio.h>
#include <l4/crtx/crt0.h>
#include <l4/util/mb_info.h>
#include <l4/names/libnames.h>
#include <l4/generic_fprov/generic_fprov-server.h>

#include "dm.h"

const int l4thread_max_threads = 4;

/**
 * Return a new dataspace with the requested file in it
 *
 * \param obj           our client
 * \param name		requested file name
 * \param dm		dataspace manager for allocating the dataspace
 * \param flags		flags used for creating the dataspace
 * \retval ds		dataspace including the file image
 * \retial size		file size
 * \retval _ev		dice-generated environment
 *
 * \return		- 0 on success
 *			- -L4_ENOMEM    memory allocation failed
 *			- -L4_ENOTFOUND file not found
 *			- -L4_EIO       error reading file
 *			- -L4_EINVAL    error passing the dataspace to client
 */
long
l4fprov_file_open_component (CORBA_Object _dice_corba_obj,
                             const char* fname,
                             const l4_threadid_t *dm_id,
                             unsigned long flags,
                             l4dm_dataspace_t *ds,
                             l4_size_t *size,
                             CORBA_Server_Environment *_dice_corba_env)
{
  return dm_open(fname, flags, *dm_id, *_dice_corba_obj, ds, size);
}


int
main(int argc, const char *argv[])
{
  l4util_mb_info_t *mbi = (l4util_mb_info_t*)crt0_multiboot_info;
  l4util_mb_mod_t  *mod;
  int i;

  if (!mbi ||
      !(mbi->flags & L4UTIL_MB_MODS) || !mbi->mods_addr || !mbi->mods_count)
    {
      printf("No modules passed -- giving up\n");
      return -1;
    }

  dm_start();

  mod = (l4util_mb_mod_t*)(l4_addr_t)mbi->mods_addr;
  printf("Passed the following modules:\n");
  for (i=0; i<mbi->mods_count; i++)
    printf("  module \"%s\" (%dkB)\n",
           (const char*)(l4_addr_t)mod[i].cmdline,
           (mod[i].mod_end-mod[i].mod_start+1023)/1024);

  if (!names_register("BMODFS"))
    {
      printf("Failed to register at name server\n");
      return -1;
    }

  l4fprov_file_server_loop(0);
}
