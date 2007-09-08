/*
 * \brief   Normal operation; process client requests.
 * \date    2006-07-17
 * \author  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 * \author  Bernhard Kauer <kauer@tudos.de>
 */
/*
 * Copyright (C) 2006      Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 * Copyright (C) 2004-2006 Bernhard Kauer <kauer@tudos.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Lyon package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/* generic includes */
#include <stdlib.h>

/* L4-specific includes */
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>

#include <l4/stpm/rand.h>
#include <l4/stpm/rsaglue.h>
#include <l4/stpm/tcg/basic.h>

/* local includes */
#include "server.h"
#include "seal_anchor.h"

#include "lyon-server.h"
#include "lyon.h"

/*
 * ***************************************************************************
 */

int
lyonif_add_component (CORBA_Object _dice_corba_obj,
                      const char parent_id[20],
                      const l4_threadid_t *new_child,
                      const char new_id[20],
                      const char* name,
                      const char* hash,
                      CORBA_Server_Environment *_dice_corba_env)
{
    return lyon_add(*_dice_corba_obj, parent_id, *new_child, new_id, name, hash);
}



int
lyonif_del_component (CORBA_Object _dice_corba_obj,
                      const char id[20],
                      CORBA_Server_Environment *_dice_corba_env)
{
    return lyon_del(*_dice_corba_obj, id);
}



int
lyonif_link_component (CORBA_Object _dice_corba_obj,
                       unsigned int keylen,
                       const char *key,
                       unsigned int *dstlen,
                       char **dst,
                       CORBA_Server_Environment *_dice_corba_env)
{
    *dstlen = lyon_link(*_dice_corba_obj, keylen, key, *dstlen, *dst);
    return *dstlen;
}



int
lyonif_quote_component (CORBA_Object _dice_corba_obj,
                        unsigned int nlen,
                        const char *nonce,
                        lyon_quote_t *quote,
                        CORBA_Server_Environment *_dice_corba_env)
{
    return lyon_quote(*_dice_corba_obj, nlen, nonce, quote);
}

int
lyonif_extend_component (CORBA_Object _dice_corba_obj,
                        unsigned int datalen,
                        const char *data,
                        CORBA_Server_Environment *_dice_corba_env)
{
    return lyon_extend(*_dice_corba_obj, datalen, data);
}


int
lyonif_seal_component (CORBA_Object _dice_corba_obj,
                       const char id[20],
                       unsigned int hashlen,
                       const char *hash,
                       unsigned int srclen,
                       const char *src,
                       unsigned int *dstlen,
                       char **dst,
                       CORBA_Server_Environment *_dice_corba_env)
{
    *dstlen = lyon_seal(*_dice_corba_obj, id, hashlen, hash, srclen, src, *dstlen, *dst);
    return *dstlen;
}



int
lyonif_unseal_component (CORBA_Object _dice_corba_obj,
                         unsigned int srclen,
                         const char *src,
                         unsigned int *dstlen,
                         char **dst,
                         CORBA_Server_Environment *_dice_corba_env)
{
    *dstlen = lyon_unseal(*_dice_corba_obj, srclen, src, *dstlen, *dst);
    return *dstlen;
}

/*
 * ***************************************************************************
 */

static int
get_root_hash(void)
{
    /* FIXME: hash the configuration of the basic platform (PCRs, ...) into
     *        the root node of the hash database */

    static const lyon_hash_t dummy_hash = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    
    return lyon_add(l4_myself(), lyon_nil_id, l4_myself(), lyon_nil_id,
                    LYON_ROOT_NAME, dummy_hash);
}

/*
 * ***************************************************************************
 */

int
lyon_start(char *srk_auth, char *secrets_auth)
{
    int  ret;
    char counter_auth[TCG_HASH_SIZE];

    DICE_DECLARE_SERVER_ENV(env);

    ret = unseal_secrets(srk_auth, secrets_auth,
                         lyon_aes_key, &lyon_rsa_key, counter_auth);
    if (ret != 0)
        return ret;

    ret = get_root_hash();
    if (ret != 0)
        return ret;
    
    if (names_register(LYONIF_NAME) == 0)
    {
        printf("Error registering at nameserver\n");
        return -1;
    }

    env.malloc = (dice_malloc_func) malloc;
    env.free   = free;

    lyonif_server_loop(&env);

    return 0;
}
