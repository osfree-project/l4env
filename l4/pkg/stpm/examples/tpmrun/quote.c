/*
 * Copyright (C) 2007 Alexander Boettcher <boettcher@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <tcg/quote.h>

#include "tpmrun.h"

/*
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
        struct tpm_pcr_value value[16]; // number of PCRs 
};

*/
int quotePCRs(unsigned int    keyhandle,
              unsigned char * passhash,
              unsigned char * nouncehash,
              unsigned char * output,
              unsigned int  * outputlen,
              unsigned char * pcrcomposite,
              unsigned int  pcrlen,
              unsigned int maxPCRs)
{
  unsigned char pcrselect[20];
  unsigned short select_count;
  unsigned long size_composite;
  unsigned long value_count;
  int res;

  // the select count must be in bytes and in big endian
  select_count = maxPCRs >> 3;
  if (maxPCRs % 8 != 0)
    select_count += 1;

  // sanity check
  if (select_count + 2 > sizeof(pcrselect))
    return -2;

  // 2 bytes select count + select bytes used +
  //  4 byte for value count + maxPCRs * 20 value bytes
  size_composite = 2 + select_count + 4 + (maxPCRs * 20);

  // sanity check
  if (size_composite > pcrlen)
    return -3;

  // set the size/count to pcrselect (the first 2 bytes)
  // convert it to network byte order (big endian)
  *((unsigned short *)pcrselect) = htons(select_count);

  // select starts after first 2 bytes
  // request all PCRs
  memset(&pcrselect[2], ~0, select_count);
    
  res= TPM_Quote(keyhandle, passhash, 
                pcrselect, 
                nouncehash, 
                pcrcomposite,
                output, outputlen);

  if (res)
    return res;

  value_count = ntohl(*(unsigned long *)&pcrcomposite[2 + select_count]);   
  
  //sanity check
  if (value_count % 20 != 0)
    return -4;

  return 0;
}
