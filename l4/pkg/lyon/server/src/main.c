/*
 * \brief   Initialize the TPM and seal the VTPM key to the VTPM server
 * \date    2006-07-04
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

/* generic includes */
#include <stdlib.h>
#include <stdio.h>

/* L4-specific includes */
#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/util/macros.h>
#include <l4/util/rdtsc.h>
#include <l4/env/errno.h>

/* local includes */
#include <l4/stpm/cryptoglue.h>
#include <l4/stpm/rsaglue.h>
#include <l4/stpm/rand.h>
#include <l4/stpm/tcg/basic.h>
#include <l4/stpm/tcg/owner.h>
#include <l4/stpm/tcg/seal.h>
#include <l4/stpm/tcg/ord.h>

#include "init.h"
#include "server.h"

/*
 * ***************************************************************************
 */

/* auth data has to be 20 bytes long */
#define OWNER_AUTH   "owner_auth0123456789"
#define SRK_AUTH     "srk_auth012345678901"
#define COUNTER_AUTH "counter_auth01234567"
#define SECRETS_AUTH "secrets_auth01234567"

/*
 * ***************************************************************************
 */

int
main(int argc, char **argv)
{
    /* FIXME: We need some GUI support here:
     *        1) GUI for the init phase (ask for 'secrets' password, print
     *           some warning and call lyon_init())
     *        2) GUI for normal operation (ask for 'secrets' password and
     *           call lyon_init()) */

#if 1
    if (lyon_init(NULL, OWNER_AUTH, SRK_AUTH, SECRETS_AUTH, COUNTER_AUTH) != 0)
        return 1;
#endif

    if (lyon_start(SRK_AUTH, SECRETS_AUTH) != 0)
        return 1;

    return 0;
}
