/*
 * \brief   Glue code to use the TPM v1.2 TIS layer with STPM
 * \date    2007-03-05
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2007  Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <unistd.h>
#include "util.h"

///////////////////////////////////////////////////

void __exit(unsigned status)
{
    _exit(status);
}
