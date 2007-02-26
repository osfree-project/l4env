INTERFACE:

#include "types.h"

namespace Page {
  
  /* These things must be defined in arch part in
     the most efficent way according to the architecture.
  
  typedef int Attribs;

  enum Attribs_enum {
    USER_NO  = xxx, ///< User No access
    USER_RO  = xxx, ///< User Read only
    USER_RW  = xxx, ///< User Read/Write
    USER_RX  = xxx, ///< User Read/Execute
    USER_XO  = xxx, ///< User Execute only
    USER_RWX = xxx, ///< User Read/Write/Execute

    NONCACHEABLE = xxx, ///< Caching is off
    CACHEABLE    = xxx, ///< Cahe is enabled
  };
     
  
  */

  
};

class PF {
public:
  static Mword is_translation_error( Mword error );
  static Mword is_usermode_error( Mword error );
  static Mword is_read_error( Mword error );
  static Mword addr_to_msgword0( Address pfa, Mword error );
  static Mword pc_to_msgword1( Address pc, Mword error );
};


IMPLEMENTATION:
//-
