INTERFACE [ia32 || ux]:

#include "types.h"
#include "l4_types.h"

EXTENSION class Utcb
{
  /* must be 2^n bytes */
public:
  enum { Max_words = 32 };
  Mword values[Max_words];
  Mword buffers[31];
  L4_timeout_pair xfer;

  /**
   * UTCB Userland Thread Control Block.
   *
   * This data structure is the userland thread control block of
   * a thread. It is modifiable from userland.
   */
public:

  enum {
    Utcb_addr_mask	= 0xfffffffc,
  };

};

// ----------------------------------------------------------------------------
IMPLEMENTATION[(ia32 || ux) && debug]:

PUBLIC
void
Utcb::print() const
{}

