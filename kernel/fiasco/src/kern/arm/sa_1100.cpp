INTERFACE:

#include "types.h"

class Sa1100 
{
public:
  enum {
    RSRR = 0xef060000 + 0x030000,
    

    RSRR_SWR = 1,
    
  };

  static void hw_reg( Mword value, Mword reg );

};


IMPLEMENTATION:

IMPLEMENT inline
void Sa1100::hw_reg( Mword value, Mword reg )
{
  *(Mword volatile*)reg = value;
}
//-
