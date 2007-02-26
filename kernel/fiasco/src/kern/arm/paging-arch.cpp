INTERFACE [arm]:

#include "types.h"

namespace Page 
{
  typedef Unsigned32 Attribs;

  enum Attribs_enum {
    KERN_RO  = 0x0000,
    USER_NO  = 0x0400, ///< User No access
    USER_RO  = 0x0800, ///< User Read only
    USER_RW  = 0x0c00, ///< User Read/Write
    USER_RX  = 0x0800, ///< User Read/Execute
    USER_XO  = 0x0800, ///< User Execute only
    USER_RWX = 0x0c00, ///< User Read/Write/Execute

    NONCACHEABLE  = 0x00, ///< Caching is off
    CACHEABLE     = 0x0c, ///< Cache is enabled

    // The next are ARM specific
    WRITETHROUGH = 0x08, ///< Write through cached
    BUFFERED     = 0x04, ///< Write buffer enabled

    MAX_ATTRIBS  = 0x0dec,
  };
};

//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:

PUBLIC static inline
Mword PF::is_alignment_error(Mword error)
{ return (error & 0x0d) == 0x01; }

IMPLEMENT inline
Mword PF::is_translation_error( Mword error )
{
  return (error & 0x0d/*FSR_STATUS_MASK*/) == 0x05/*FSR_TRANSL*/;
}

IMPLEMENT inline
Mword PF::is_usermode_error( Mword error )
{
  return (error & 0x00010000/*PF_USERMODE*/);
}

IMPLEMENT inline
Mword PF::is_read_error( Mword error )
{
  return (error & 0x00020000/*PF_WRITE*/);
}

IMPLEMENT inline
Mword PF::addr_to_msgword0( Address pfa, Mword error )
{
  Mword a = pfa & ~3;
  if(is_translation_error( error ))
    a |= 1;
  if(!is_read_error(error))
    a |= 2;
  return a;
}

IMPLEMENT inline
Mword PF::pc_to_msgword1( Address pc, Mword error )
{
  if(is_usermode_error(error))
    return pc;
  else 
    return (Mword)-1;
}

