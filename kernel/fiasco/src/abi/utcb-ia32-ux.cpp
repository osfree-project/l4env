INTERFACE [v2-utcb]:
class Utcb
{};

INTERFACE [{ia32,ux}-utcb]:

#include "types.h"
#include "l4_types.h"

EXTENSION class Utcb
{
  /* must be 2^n bytes */
public:
  Mword		status;
  Mword		values[16];
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

private:
  /*
    The complete v2 L4 id, needed for fallback to kernel ipc
    and for V2 ABI compatibility
  */
  Global_id	_id;

  /* User sp and ip, if the thread is in ipc wait. */
  Address	_esp;
  Address	_eip;

  /* It contains the ipc partner, ipc state and the thread lock bit */
  Mword		_state;
  /* Contains fpu and sendqueue bit */
  Mword		_snd_state;

private:
  Unsigned32	_padding[7];
};

// ----------------------------------------------------------------------------
IMPLEMENTATION[{ia32,ux}-v2-utcb-debug]:

PUBLIC
void
Utcb::print() const
{}
