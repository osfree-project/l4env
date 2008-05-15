/*
 * \brief   Macro to instantiate IDL client stubs.
 * \date    2004-11-12
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2004  Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef  _ENCAP_H
#define  _ENCAP_H

#include <l4/sys/linkage.h>

L4_CV int stpm_transmit ( const char *write_buf, unsigned int write_count,
                          char **read_buffer, unsigned int *read_count);

L4_CV int stpm_abort (void);

L4_CV int stpm_check_server(char * name, int recheck);

#endif
