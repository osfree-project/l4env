/*
 * \brief   A fake random number generator.
 * \date    2006-07-28
 * \author  Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006 Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

/* L4-specific includes */
#include <l4/crypto/random.h>

static char base = 0;

/**
 * Fill the buffer with num random bytes.
 * 
 * FIXME: This is a dummy function until real random is
 *        available. We should use TPM_GetRandom() ...
 */
int 
rand_buffer(char *buffer, int len)
{
  int i;
  
  for (i = 0; i < len; i++)
    buffer[i] = base++;
  
  return len;
}


/**
 * Needed by libcrypto.
 */
int
crypto_randomize_buf(char *buf, unsigned int len)
{
    return rand_buffer(buf, len);
}

