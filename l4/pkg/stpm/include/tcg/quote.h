/*
 * \brief   Header for quote functions.
 * \date    2004-06-02
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2002  Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef _QUOTE_H
#define _QUOTE_H


struct tpm_pcr_selection2
{
	unsigned short size;
	unsigned short select;
};

struct tpm_pcr_value
{
	unsigned char value[20];
};


struct tpm_pcr_composite
{
	struct tpm_pcr_selection2 select;
	unsigned long valuesize;
	struct tpm_pcr_value value[16];
};


unsigned long 
STPM_Quote(unsigned long keyhandle,
	  unsigned char *keyauth,
	  unsigned char *pcrselect,
	  unsigned char *nonce,
	  unsigned char *pcrcomposite,
	  unsigned char *blob, 
	  unsigned int *bloblen);
#ifndef TPM_Quote
#define TPM_Quote(...) STPM_Quote(__VA_ARGS__)
#endif

int quote_stdout(int argc, unsigned char *argv[]);

#endif
