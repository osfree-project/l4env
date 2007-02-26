INTERFACE [arm-x0-utcb]:

#include "types.h"

class Utcb
{
  /* must be 2^n bytes */
public:
  Mword status;     /* Mword size so that all values are mword aligned */
  Mword values[20];
  Mword snd_size;
  Mword rcv_size;
  Mword _padding[9];
};

