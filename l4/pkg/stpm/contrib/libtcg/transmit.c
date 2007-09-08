/*
 * \brief   TPM transmit function for STPM service.
 * \date    2004-06-02
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


#include <stdio.h>
#include <tcg/tpm.h>
#include <netinet/in.h> // ntohl
#include "stpmif.h"

#define DEBUG_BLOB 0

unsigned long
TPM_Transmit(unsigned char *blob,char *msg)
{
        unsigned int size;
	int res;

	// printf("> %s: %s\n",__func__,msg);
	size = ntohl(*(unsigned int *) & blob[TCG_PARAMSIZE_OFFSET]);
	unsigned read_count = TCG_MAX_BUFF_SIZE;
	char **read_blob = (char **)&blob;
#if DEBUG_BLOB
        LOG_printf("> %s", msg);
        PRINT_HASH_SIZE(blob, size);
#endif
	if (0 >= (res = stpm_transmit((char*)blob, size, (char**)read_blob, &read_count)))
	  {
	    printf("TPM transmit Error: %d\n", res);
	    return res;
	  }
#if DEBUG_BLOB
        LOG_printf("< %s", msg);
        PRINT_HASH_SIZE(blob, read_count);
#endif
	return ntohl(*(unsigned int *) & blob[TCG_RETURN_OFFSET]);
}
