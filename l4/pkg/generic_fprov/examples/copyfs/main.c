/**
 * \file  generic_fprov/examples/copyfs/main.c
 * \brief File provider that offers copies of files provided
 *        by other file providers (i.e., once served, the file is
 *        guaranteed not to be modified by the other file provider).
 *
 * \date    08/2007
 * \author  Carsten Weinhold <weinhold@os.inf.tu-dresden.de> */

/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <stdio.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/env/errno.h>
#include <l4/generic_fprov/generic_fprov-client.h>
#include <l4/generic_fprov/generic_fprov-server.h>

/*
 * ************************************************************************
 */

static l4_threadid_t real_fprov_id;

/*
 * ************************************************************************
 */

/**
 * Return a new dataspace with the requested file in it
 *
 * \param obj           our client
 * \param name          requested file name
 * \param dm            dataspace manager for allocating the new dataspace
 * \param flags         flags used for creating the dataspace
 * \retval ds           dataspace including the file image
 * \retial size         file size
 * \retval _ev          dice-generated environment
 *
 * \return  - 0 on success
 *          - -L4_ENOMEM    memory allocation failed
 *          - -L4_ENOTFOUND file not found
 *          - -L4_EIO       error reading file
 *          - -L4_EINVAL    error passing the dataspace to client
 */
long
l4fprov_file_open_component(CORBA_Object _dice_corba_obj,
                            const char* fname,
                            const l4_threadid_t *dm_id,
                            unsigned long flags,
                            l4dm_dataspace_t *ds,
                            l4_size_t *size,
                            CORBA_Server_Environment *_dice_corba_env)
{
  DICE_DECLARE_ENV(env);
  l4dm_dataspace_t tmp_ds;
  void *tmp_addr, *addr;
  long error;

  error = l4fprov_file_open_call(&real_fprov_id, fname, dm_id, flags,
                                 &tmp_ds, size, &env);

  if (DICE_HAS_EXCEPTION(&env))
    return -L4_EIPC;
  if (error < 0)
    return error;

  if ((error = l4dm_mem_open(*dm_id, *size, 0, flags, fname, ds)))
    {
      LOG("Error %ld allocating memory for file %s", error, fname);
      l4dm_close(&tmp_ds);
      return error;
    }

  if ((error = l4rm_attach(ds, *size, 0, L4DM_RW, &addr)))
    {
      LOG("Error %ld attaching dataspace for file %s", error, fname);
      l4dm_close(ds);
      l4dm_close(&tmp_ds);
      return error;
    }

  if ((error = l4rm_attach(&tmp_ds, *size, 0, L4DM_RO, &tmp_addr)))
    {
      LOG("Error %ld attaching dataspace for module %s", error, fname);
      l4rm_detach(addr);
      l4dm_close(ds);
      l4dm_close(&tmp_ds);
      return error;
    }

  memcpy(addr, tmp_addr, *size);

  l4rm_detach(tmp_addr);
  l4dm_close(&tmp_ds);

  l4rm_detach(addr);
  l4dm_transfer(ds, *_dice_corba_obj);

  return 0;
}

int
main(int argc, const char *argv[])
{
  char proxy_name[NAMES_MAX_NAME_LEN];

  if (argc < 2)
    {
      LOG("No file provider specified to fetch files from.");
      return -1;
    }

  if (!names_waitfor_name(argv[1], &real_fprov_id, 5000))
    {
      LOG("Failed to lookup specified file provider at names.");
      return -1;
    }


  snprintf(proxy_name, NAMES_MAX_NAME_LEN, "COPYFS:%s", argv[1]);
  proxy_name[NAMES_MAX_NAME_LEN-1] = 0;

  if (!names_register(proxy_name))
    {
      LOG("Failed to register at name server");
      return -1;
    }

  l4fprov_file_server_loop(0);
}
