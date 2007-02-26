/*!
 * \file   dietlibc/lib/backends/l4env_base/abort.c
 * \brief  Own abort implementation, as the dietlibc-one relies on signals
 *
 * \date   08/19/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <stdlib.h>

void abort(void) {
    /* We do not provide signals, hence no SIGABRT handling here */
    exit(1);
}
