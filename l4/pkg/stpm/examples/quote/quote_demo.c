/*
 * \brief   Example demostrating TPM_Quote().
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
#include <stdlib.h>
#include <netinet/in.h>
#include <tcg/basic.h>
#include <tcg/quote.h>
#include <tcg/hmac.h>

int l4libc_heapsize = 64*1024;
 
int main(int argc, char *argv[])
{
	int i;
	unsigned char output[20];

	printf("argc: %d\n",argc);
	for (i=0;i<argc;i++)
		printf("argv[%d] = '%s'\n",i,argv[i]);


	sha1((unsigned char*)argv[2], strlen(argv[2]), output);

	printf("hash done\n");

	return quote_stdout(argc,(unsigned char **)argv);
}
