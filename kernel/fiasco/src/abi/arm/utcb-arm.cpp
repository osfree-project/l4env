INTERFACE [arm]:

#include "l4_types.h"
#include "types.h"

EXTENSION class Utcb
{
  /* must be 2^n bytes */
public:
  enum { Max_words = 32 };
  Mword values[Max_words];
  Mword buffers[31];
  L4_timeout_pair xfer;
};
