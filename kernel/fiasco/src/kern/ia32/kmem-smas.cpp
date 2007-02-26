INTERFACE:

#include <flux/x86/seg.h>
#include "atomic.h"

EXTENSION class Kmem
{
private:
  static unsigned int current_user_gdt;    //current size of user gdt entry
};

IMPLEMENTATION[smas]:

unsigned int Kmem::current_user_gdt;

/** Set user data and code segment.
 *  Segments are set only if the content would really change.
 *  This is all bitswitching to be effective, will be explained soon. 
 *  @return true, if GDT has been changed.
 */
PUBLIC inline static
void Kmem::set_gdt_user( unsigned gdtinfo )
{
  if ( gdtinfo != current_user_gdt ) {
  
    current_user_gdt = gdtinfo;

    unsigned *gdtptr = reinterpret_cast<unsigned *>(gdt);
    gdtptr[gdt_code_user/4] = gdtptr[gdt_data_user/4] 
                                = (gdtinfo & 0xF000) | 0xFFF ;
    gdtinfo = ( gdtinfo & 0xFF0F00FF) | 0xC0FB00;
    gdtptr[gdt_code_user/4 + 1] = gdtinfo;
    gdtinfo &= ~0x800;
    gdtptr[gdt_data_user/4 + 1] = gdtinfo;


    set_gs( gdt_data_user | SEL_PL_U );
    set_fs( gdt_data_user | SEL_PL_U );

  }
}


/** Updates the small address space window and increments
 *  the version counter.
 *  @return Version of the update.
 */
PUBLIC static inline unsigned
Kmem::update_smas_window( vm_offset_t addr, pd_entry_t entry, bool flush )
{
  unsigned index = (addr >> PDESHIFT) & PDEMASK;

  kdir[index] = entry;

  return (flush)?inc_smas_version():
            kdir[(reinterpret_cast<unsigned long>(&_smas_version_1) >>          PDESHIFT) & PDEMASK];

}

/* Increment version of small space window in master copy.
 * */
PUBLIC static inline unsigned
Kmem::inc_smas_version( ) {

  unsigned *version = &(kdir[(reinterpret_cast<unsigned long>(&_smas_version_1) >> PDESHIFT) & PDEMASK]);
  unsigned old;

  // Be careful here. must not be INTEL_PDE_VALID
  // Therefore versions are always incremented by 2.  
  do {
    old = *version;
  } while ( !smp_cas( version, old, old + 2) );

  return old + 2;

}

