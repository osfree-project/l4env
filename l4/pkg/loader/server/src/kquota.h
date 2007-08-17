/******************************************************************************
 * \file        loader/server/src/kquota.h
 * \brief       kernel memory quotas
 *
 * \date        08-2007
 * \author      Bjoern Doebel <doebel@os.inf.tu-dresden.de>
 *
 * (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
*****************************************************************************/

#ifndef __LOADER_SERVER_SRC_KQUOTA_H__
#define __LOADER_SERVER_SRC_KQUOTA_H__

#include <l4/sys/types.h>

#define KQUOTA_NAME_LENGTH  20

/** Users of a kernel memory quota */
typedef struct kquota_user
{
  l4_threadid_t       tid;
  struct kquota_user  *next;
} kquota_user_t;

/** Kernel memory quota descriptor. */
typedef struct cfg_kquota_t
{
  char                name[KQUOTA_NAME_LENGTH]; /**< This quota's name. */
  unsigned            size;                     /**< This quota's size
                                                     in Quota Base Units */
  kquota_user_t       *users;                   /**< Users of this kquota. */
  struct cfg_kquota_t *next;                    /**< Queuing information. */
} cfg_kquota_t;

/** List of all known quotas */
extern cfg_kquota_t *kquota_list;

/** Add quota to the list of quota descriptors.
 *
 * \param name      unique quota name
 * \param size      size of quota in Quota Base Units
 *
 * \return  0       success
 *          <0      error
 */
int add_kquota(char *name, unsigned size);

/** Get quota with a given name.
 *
 * \param name      unique quota name
 * \return NULL     no such quota
 *         quota    pointer to quota descriptor
 */
cfg_kquota_t *get_kquota(const char *name);

/** Add user to a kquota.
 * \param user      User L4 thread ID
 * \return  0       success
 *          <0      error
 */
int kquota_add_user(cfg_kquota_t *kq, l4_threadid_t user);

/** Remove user from kquota.
 * \param user      User L4 thread ID
 */
void kquota_del_user(cfg_kquota_t *kq, l4_threadid_t user);

#endif /* ! __LOADER_SERVER_SRC_KQUOTA_H__ */
