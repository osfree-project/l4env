INTERFACE [arm]:

class Paging
{};

//-----------------------------------------------------------------------------
INTERFACE [arm && armv5]:

#include "types.h"

namespace Page 
{
  typedef Unsigned32 Attribs;

  enum Attribs_enum {
    KERN_RW  = 0x0400, ///< User No access
    USER_RO  = 0x0000, ///< User Read only
    USER_RW  = 0x0c00, ///< User Read/Write

    USER_BIT = 0x00800,

    Cache_mask    = 0x0c,
    NONCACHEABLE  = 0x00, ///< Caching is off
    CACHEABLE     = 0x0c, ///< Cache is enabled

    // The next are ARM specific
    WRITETHROUGH = 0x08, ///< Write through cached
    BUFFERED     = 0x04, ///< Write buffer enabled

    MAX_ATTRIBS  = 0x0dec,
    Local_page   = 0,
  };
};


//----------------------------------------------------------------------------
INTERFACE [arm && armv6]:

#include "types.h"

namespace Page 
{
  typedef Unsigned32 Attribs;

  enum Attribs_enum {
    KERN_RO  = 0x0210,
    KERN_RW  = 0x0010, ///< User No access
    USER_RO  = 0x0230, ///< User Read only
    USER_RW  = 0x0030, ///< User Read/Write

    USER_BIT = 0x0020,

    Cache_mask    = 0x1cc,
    NONCACHEABLE  = 0x000, ///< Caching is off
    CACHEABLE     = 0x04c, ///< Cache is enabled

    // The next are ARM specific
    WRITETHROUGH = 0x08, ///< Write through cached
    BUFFERED     = 0x04, ///< Write buffer enabled

    MAX_ATTRIBS  = 0x0ffc,
    Local_page   = 0x800,
  };
};


//---------------------------------------------------------------------------
IMPLEMENTATION [arm]:


/* this functions do nothing on the ARM architecture */
PUBLIC static inline
Address
Paging::canonize(Address addr)
{
  return addr;
}

PUBLIC static inline
Address
Paging::decanonize(Address addr)
{
  return addr;
}

//---------------------------------------------------------------------------

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
  return (error & 0x00020000/*PF_READ*/);
}

IMPLEMENT inline
Mword PF::write_error()
{ return 0x00000005; }

IMPLEMENT inline
Mword PF::usermode_error()
{ return 0x00010000; }

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

