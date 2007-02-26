INTERFACE:

#include "types.h"

namespace Page {

  typedef Unsigned32 Attribs;

  enum Attribs_enum {

    USER_NO  = 0x06, ///< User No access
    USER_RO  = 0x00, ///< User Read only
    USER_RW  = 0x02, ///< User Read/Write
    USER_RX  = 0x00, ///< User Read/Execute
    USER_XO  = 0x00, ///< User Execute only
    USER_RWX = 0x02, ///< User Read/Write/Execute

    RO       = 0x00,
    KERN     = 0x04,
    USER     = 0x00,

    CPU_GLOBAL = 0x100, ///< pinned in the TLB

    NONCACHEABLE  = 0x10, ///< Caching is off
    CACHEABLE     = 0x00, ///< Cache is enabled

    WRITE_THROUGH = 0x08,

    MAX_ATTRIBS   = 0x11e
  };


};

IMPLEMENTATION[ia32-ux]:

#include "regdefs.h"

IMPLEMENT inline NEEDS["regdefs.h"]
Mword PF::is_translation_error( Mword error )
{
  return !(error & PF_ERR_PRESENT);
}

IMPLEMENT inline NEEDS["regdefs.h"]
Mword PF::is_usermode_error( Mword error )
{
  return (error & PF_ERR_USERMODE);
}


IMPLEMENT inline NEEDS["regdefs.h"]
Mword PF::is_read_error( Mword error )
{
  return !(error & PF_ERR_WRITE);
}

IMPLEMENT inline NEEDS["regdefs.h"]
Mword PF::addr_to_msgword0( Address pfa, Mword error )
{
  return    (pfa   & ~(PF_ERR_PRESENT | PF_ERR_WRITE))
          | (error &  (PF_ERR_PRESENT | PF_ERR_WRITE));
}

IMPLEMENT inline NEEDS["regdefs.h"]
Mword PF::pc_to_msgword1( Address pc, Mword error )
{
  return is_usermode_error(error) ? pc : (Mword)-1;
}
