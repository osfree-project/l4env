INTERFACE [32bit]:

#include "types.h"

EXTENSION class L4_fpage
{
public:
  /**
   * Flex page constants.
   */
  enum
  {
    Whole_space = 32, ///< Size of the whole address space.
  };

private:

  enum
  {
    /* +--- 32-12 ---+ 11-10 +- 9-8 -+- 7-2 + 1 + 0 +
     * | page number |   C   | unused | size | W | G |
     * +-------------+-------+--------+------+---+---+ */
    Size_mask        = 0x000000fc,
    Size_size        = 6,
    Cache_type_mask  = 0x00000e00,///< C (Cache type [extension])
    Page_mask        = 0xfffff000,
    Special_fp_mask  = 0xf0000f02,
    All_spaces_id    = 0xf0000f00,
  };

};

//---------------------------------------------------------------------------
IMPLEMENTATION [32bit]:

//-
