INTERFACE [v2-utcb]:
class Utcb
{};

INTERFACE [(ia32|ux) & utcb]:

#include "types.h"
#include "l4_types.h"

EXTENSION class Utcb
{
  /* must be 2^n bytes */
public:
  enum { Max_words = 29 };
  Mword		status;
  Mword		values[Max_words];
  Unsigned32	snd_size;
  Unsigned32	rcv_size;

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
IMPLEMENTATION[{ia32,ux}-v2-utcb-debug]:

PUBLIC
void
Utcb::print() const
{}
