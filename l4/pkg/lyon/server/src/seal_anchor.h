/*
 * \brief   Header for TPM seal and unseal functionality.
 * \date    2006-07-14
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

#ifndef __LYON_SEAL_ANCHOR_H
#define __LYON_SEAL_ANCHOR_H

#include <l4/stpm/rsaglue.h>

/*
 * ***************************************************************************
 */

int
seal_secrets(const char *srk_auth, const char *secrets_auth,
             const char *aes_key, rsa_key_t *rsa_key, char *counter_auth);

int
unseal_secrets(char *srk_auth, char *secrets_auth,
               char *aes_key_out, rsa_key_t *rsa_key_out,
               char *counter_auth_out);

#endif /* __LYON_SEAL_ANCHOR_H */
