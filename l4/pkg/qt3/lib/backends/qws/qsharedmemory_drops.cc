/* $Id$ */
/*****************************************************************************/
/**
 * \file   lib/backends/qws/qsharedmemory_drops.cc
 * \brief  L4-specific QSharedMemory implementation.
 *
 * \date   11/02/2004
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004-2006 Technische Universitaet Dresden
 * This file is part of the Qt3 port for L4/DROPS, which is distributed under
 * the terms of the GNU General Public License 2. Please see the COPYING file
 * for details.
 */

#ifdef DEBUG
# define _DEBUG 1
# error sdfdfg 
#else
# define _DEBUG 0
#endif

/*** L4-SPECIFIC INCLUDES ***/
#include <l4/dm_generic/consts.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/l4rm/l4rm.h>
#include <l4/log/l4log.h>

#include <l4/qt3/l4qws_shm_client.h>

/*** GENERAL INCLUDES ***/
#include <stdio.h>
#include <private/qsharedmemory_p.h>


/* **************************************************************** */

class QShmData {
public:
  l4qws_key_t      key;
  l4dm_dataspace_t ds;
};

/* **************************************************************** */

QSharedMemory::QSharedMemory (int size, const QString &fn, char c)
{
  data      = new QShmData;
  shmSize   = size;
  shmBase   = NULL;
  data->key = l4qws_key(fn.latin1(), c);
  data->ds  = L4DM_INVALID_DATASPACE;
}



QSharedMemory::~QSharedMemory ()
{
  delete data;
}



bool QSharedMemory::create ()
{
  int ret;

  ret = l4qws_shm_create_call(&l4qws_shm_server, data->key, shmSize,
                              &data->ds, &l4qws_dice_shm_env);
  if (ret != 0) {
    // already exists? let's see if we can get() it
    if (l4qws_shm_get_call(&l4qws_shm_server, data->key,
                           &data->ds, &l4qws_dice_shm_env) != 0)
      return false;
  }

  return true;
}



void QSharedMemory::destroy ()
{
  if ( !l4dm_is_invalid_ds(data->ds))
    l4qws_shm_destroy_call(&l4qws_shm_server, data->key, &l4qws_dice_shm_env);
}



bool QSharedMemory::attach ()
{
  int ret;

  if (l4dm_is_invalid_ds(data->ds)) {
    ret = l4qws_shm_get_call(&l4qws_shm_server, data->key,
                             &data->ds, &l4qws_dice_shm_env);
    if (ret != 0)
      return false;
  }
  
  if (shmSize == 0) {
    l4_size_t s;
    if (l4dm_mem_size(&data->ds, &s) == 0)
      shmSize = s;
    else
      LOGd(_DEBUG, "Could not determine size of shm region");
  }

  ret = l4rm_attach(&data->ds, shmSize, 0, L4DM_RW, &shmBase);
  LOGd(_DEBUG, "shmBase=%p", shmBase);
  
  return ret == 0;
}



void QSharedMemory::detach ()
{
  if ( ! l4dm_is_invalid_ds(data->ds) &&
       l4rm_detach(shmBase) == 0)
    data->ds = L4DM_INVALID_DATASPACE;
}



int QSharedMemory::size ()
{
  l4_size_t l4_size;

  if ( !l4dm_is_invalid_ds(data->ds) &&
       l4dm_mem_size(&data->ds, &l4_size) == 0)
    return l4_size;
  
  return -1;
}



void QSharedMemory::setPermissions(mode_t mode)
{
}

