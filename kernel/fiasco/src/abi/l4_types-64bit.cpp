INTERFACE [64bit]:

#include "types.h"

EXTENSION class L4_fpage
{
public:
  /**
   * Flex page constants.
   */
  enum
  {
    Whole_space = 64, ///< Size of the whole address space.
  };

private:

  enum 
  { 
    /* +--- 63-12 ---+ 11-10 +- 9-8 -+- 7-2 + 1 + 0 +
     * | page number |   C   | unused | size | W | G |
     * +-------------+-------+--------+------+---+---+ */
    Size_mask        = 0x00000000000001fcUL,
    Size_size        = 7,
    Cache_type_mask  = 0x0000000000000e00UL,///< C (Cache type [extension])
    Page_mask        = 0xfffffffffffff000UL,
    Special_fp_mask  = 0xfffffffff0000f02UL,
    All_spaces_id    = 0xfffffffff0000f00UL,
  };

};

//---------------------------------------------------------------------------
IMPLEMENTATION [64bit]:

//-
