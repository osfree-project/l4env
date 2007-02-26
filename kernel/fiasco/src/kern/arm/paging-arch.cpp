INTERFACE:

#include "types.h"

namespace Page {
  
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
    WRITETHROUGH = 0x08, ///< Write throught cached
    BUFFERED     = 0x04, ///< Write buffer enabled

    MAX_ATTRIBS  = 0x0dec,
  };

};


IMPLEMENTATION[arch]:

//-
