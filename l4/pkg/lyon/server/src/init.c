/*
 * \brief   Prepare the TPM (TakeOwnership(), CreateCounter(), ...).
 * \date    2006-07-14
 * \author  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 * \author  Bernhard Kauer <kauer@tudos.de>
 */
/*
 * Copyright (C) 2006  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
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
#include <l4/crypto/aes.h>

#include <l4/stpm/rsaglue.h>
#include <l4/stpm/rand.h>
#include <l4/stpm/tcg/basic.h>
#include <l4/stpm/tcg/owner.h>

/* local includes */
#include "seal_anchor.h"

/*
 * ***************************************************************************
 */
 
#define RSA_KEY_BITS 1024

/*
 * ***************************************************************************
 */

static int
create_keys(char *aes_key_out, unsigned int aes_key_bits,
            rsa_key_t *rsa_key_out, unsigned int rsa_key_bits)
{
    static char buf[1024];
    int ret;

    /* step 1/2: generate AES key */
    rand_buffer(aes_key_out, aes_key_bits / 8);
    
    /* step 2/2: generate RSA key */
    rsa_initrandom(rsa_key_out);

    ret = 1;
    while (ret > 0)
    {
        rand_buffer(buf, sizeof(buf));
        ret = rsa_insertrandom(rsa_key_out, sizeof(buf), (unsigned char *) buf);
    }

    return rsa_create(rsa_key_out, rsa_key_bits);
}


/*
 * ***************************************************************************
 */

int
lyon_init(char *old_owner_auth, char *owner_auth, char *srk_auth,
          char *secrets_auth, char *counter_auth)
{
    char      aes_key[AES128_KEY_SIZE];
    rsa_key_t rsa_key;
    keydata   srk;
    int       ret;
    
    ret = TPM_SelfTestFull();
    LOG("TPM_SelfTestFull(): ret=%d", ret);
    if (ret != 0)
        return -1;

    if (old_owner_auth)
    {
        ret = TPM_OwnerClear((unsigned char *) old_owner_auth);
        LOG("TPM_OwnerClear(): ret=%d", ret);
        if (ret != 0)
            return -1;
    }

    if (owner_auth)
    {
        ret = TPM_TakeOwnership((unsigned char *) owner_auth,
                                (unsigned char *) srk_auth, &srk);
        LOG("TPM_TakeOwnership(): ret=%d", ret);
        if (ret != 0)
            return -1;
    }

    /* FIXME: allocate monotonic counters */

    if (create_keys(aes_key, sizeof(aes_key) * 8, &rsa_key, RSA_KEY_BITS) != 0)
        return -1;

    if (seal_secrets(srk_auth, secrets_auth, aes_key, &rsa_key, counter_auth) != 0)
        return -1;

    return (ret != 0) ? -1 : 0;
}

