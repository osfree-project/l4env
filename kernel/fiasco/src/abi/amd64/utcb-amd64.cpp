INTERFACE [v2-utcb]:
class Utcb
{};

INTERFACE [amd64-utcb]:

#include "types.h"
#include "l4_types.h"

EXTENSION class Utcb
{
  /* must be 2^n bytes */
public:
  enum { Max_words = 24 };
  Mword		status;
  Mword		values[Max_words];
  Mword	        snd_size;
  Mword         rcv_size;

  /**
   * UTCB Userland Thread Control Block.
   *
   * This data structure is the userland thread control block of
   * a thread. It is modifiable from userland.
   */
public:

  enum {
    Handle_exception    = 1,
    Inherit_fpu         = 2,
    Transfer_fpu	= 4,
    Utcb_addr_mask	= 0xfffffffc,
  };

};

// ----------------------------------------------------------------------------
IMPLEMENTATION[amd64-v2-utcb-debug]:

PUBLIC
void
Utcb::print() const
{}
