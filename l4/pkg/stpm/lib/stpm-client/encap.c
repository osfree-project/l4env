/*
 * \brief   IDL client stubs of STPM interface.
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

#include <l4/stpm/stpm-client.h>
#include <l4/stpm/stpmif.h>
#include <l4/stpm/encap.h>

CHECK_SERVER(stpmif_name);

ENCAP_FUNCTION(stpm,
	       transmit,
	       (const char *write_buf, unsigned int write_count,
                char **read_buffer, unsigned int *read_count),
	       write_buf, write_count, read_buffer, read_count);

ENCAP_FUNCTION(stpm,
	       abort, 
	       (void));
