INTERFACE:

#include "types.h"

template< unsigned long Flush_area = 0, bool Ram = false >
class Mmu
{
public:
  /**
   * Clean the entire dcache.
   */
  static void clean_dcache();
  

  /**
   * Clean given D cache region.
   */
  static void clean_dcache(void const *start, void const *end);
  /**
   * Clean and invalidate the entire cache.
   * D and I cache is cleaned and invalidated and the write buffer is
   * drained.
   */
  static void flush_cache();
  

  /**
   * Clean and invalidate the given cache region.
   * D and I cache are affected.
   */
  static void flush_cache(void const *start, void const *end);

  /**
   * Clean and invalidate the entire D cache.
   */
  static void flush_dcache();

  /**
   * Clean and invalidate the given D cache region.
   */
  static void flush_dcache(void const *start, void const *end);

  /**
   * Invalidate the given D cache region.
   */
  static void inv_dcache(void const *start, void const *end);

  /**
   * Switch page table and do the necessary things. 
   */
  static void switch_pdbr(Address base);
  
 // static void write_back_data_cache(bool ram = false);
 // static void write_back_data_cache(void *a);
};

IMPLEMENTATION [!arm]:

IMPLEMENT inline
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::flush_cache()
{}

IMPLEMENT inline
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::flush_cache(void const *start, void const *end)
{}

IMPLEMENT inline
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::clean_dcache()
{}

IMPLEMENT inline
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::clean_dcache(void const *start, void const *end)
{}

IMPLEMENT inline
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::flush_dcache()
{}

IMPLEMENT inline
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::flush_dcache(void const *start, void const *end)
{}

IMPLEMENT inline
template< unsigned long Flush_area, bool Ram >
void Mmu<Flush_area, Ram>::inv_dcache(void const *start, void const *end)
{}


