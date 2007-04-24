INTERFACE [arm-x0]:

#include "types.h"

class Utcb
{
  /* must be 2^n bytes */
public:
  enum { Max_words = 29 };
  Mword status;     /* Mword size so that all values are mword aligned */
  Mword values[Max_words];
  Mword snd_size;
  Mword rcv_size;

  enum {
    Handle_exception    = 1,
    Inherit_fpu         = 2,
    Transfer_fpu	= 4,
  };
};

