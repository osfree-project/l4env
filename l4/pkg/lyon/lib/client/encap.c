/*
 * \brief   Macro-encoded client IDL stubs for Lyon interface.
 * \date    2006-07-17
 * \author  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Lyon package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <l4/lyon/lyon-client.h>
#include <l4/lyon/lyon.h>
#include <l4/env/errno.h>
#include <l4/names/libnames.h>

const lyon_id_t lyon_nil_id = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static l4_threadid_t server_id = L4_INVALID_ID;       

static int check_lyon_server (void)
{							
  if (l4_is_invalid_id(server_id))
    {							
      if (!names_waitfor_name(LYONIF_NAME, &server_id, 10000))	
	return 1;					
    }							
  return 0;						
}

int lyon_add ( l4_threadid_t owner,
                const lyon_id_t parent_id,
                l4_threadid_t new_child,
                const lyon_id_t new_id,
                const char *name,
                const char *hash)
{
  DICE_DECLARE_ENV(env);

  if (check_lyon_server()) return -L4_EINVAL;

  return lyonif_add_call(&server_id, parent_id, &new_child, new_id, name, hash, &env);

}

int lyon_del(l4_threadid_t owner,
             const lyon_id_t new_id)
{
  DICE_DECLARE_ENV(env);

  if (check_lyon_server()) return -L4_EINVAL;

  return lyonif_del_call(&server_id, new_id, &env);
}

int lyon_link(l4_threadid_t client,
              unsigned int keylen,
              const char *key,
              unsigned int dstlen,
              char *dst)
{
  DICE_DECLARE_ENV(env);

  if (check_lyon_server()) return -L4_EINVAL;

  return lyonif_link_call(&server_id, keylen, key, &dstlen, &dst, &env);
}

int lyon_quote(l4_threadid_t client,
               unsigned int nlen,
               const char *nonce,
               lyon_quote_t *quote)
{
  DICE_DECLARE_ENV(env);

  if (check_lyon_server()) return -L4_EINVAL;

  return lyonif_quote_call(&server_id, nlen, nonce, quote, &env);
}

int lyon_extend(l4_threadid_t client,
                unsigned int datalen,
                const char *data)
{
  DICE_DECLARE_ENV(env);

  if (check_lyon_server()) return -L4_EINVAL;

  return lyonif_extend_call(&server_id, datalen, data, &env);
}

int lyon_seal(l4_threadid_t client,
              const lyon_id_t id,
              unsigned int hashlen,
              const char *hash,
              unsigned int srclen,
              const char *src,
              unsigned int dstlen,
              char *dst)
{
  DICE_DECLARE_ENV(env);

  if (check_lyon_server()) return -L4_EINVAL;

  return lyonif_seal_call(&server_id, id, hashlen, hash, srclen, src,
                          &dstlen, &dst, &env);
}

int lyon_unseal(l4_threadid_t client,
                unsigned int srclen,
                const char *src,
                unsigned int dstlen,
                char *dst)
{
  DICE_DECLARE_ENV(env);

  if (check_lyon_server()) return -L4_EINVAL;

  return lyonif_unseal_call(&server_id, srclen, src, &dstlen, &dst, &env);
}
