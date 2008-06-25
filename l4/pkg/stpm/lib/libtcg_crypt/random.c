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
#include <l4/stpm/tcg/rand.h>
#include <l4/log/l4log.h>

/**
 * Fill the buffer with num random bytes.
 */
int 
rand_buffer(unsigned char *buffer, int len)
{
  int error;
  unsigned long count;
 
  error = STPM_GetRandom(len, &count, buffer);
  if (error != 0 || count != len)
  {
    LOG("Error generating random numbers, requested %d, got %lu, errorcode %d", len, count, error);
    return -1;
  }
  
  return count;
}


/**
 * Needed by libcrypto.
 */
int
crypto_randomize_buf(char *buf, unsigned int len)
{
    return rand_buffer(buf, len);
}

