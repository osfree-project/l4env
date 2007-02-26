INTERFACE:


EXTENSION class Kmem
{
private:
  //current size of user gdt entry   
  static unsigned int volatile current_user_gdt asm ("KMEM_CURRENT_GDT");    
};

IMPLEMENTATION[ia32-smas]:

#include "atomic.h"
#include <flux/x86/seg.h>

unsigned int volatile Kmem::current_user_gdt;

/** Set user data and code segment.
 *  Segments are set only if the content would really change.
 *  This is all bitswitching to be effective, will be explained soon. 
 *  @return true, if GDT has been changed.
 */
PUBLIC static inline NEEDS[<flux/x86/seg.h>]
void 
Kmem::set_gdt_user (unsigned gdtinfo)
{
  if (gdtinfo != current_user_gdt) 
    {
      current_user_gdt = gdtinfo;
  
      unsigned volatile *gdtptr =
                 reinterpret_cast<unsigned volatile *>(gdt);
      gdtptr[gdt_code_user/4] = gdtptr[gdt_data_user/4] 
                                  = (gdtinfo & 0xF000) | 0xFFF ;
      gdtinfo = ( gdtinfo & 0xFF0F00FF) | 0xC0FB00;
      gdtptr[gdt_code_user/4 + 1] = gdtinfo;
      gdtinfo &= ~0x800;
      gdtptr[gdt_data_user/4 + 1] = gdtinfo;
  
      set_fs( gdt_data_user | SEL_PL_U );
    }
}

/** Updates the small address space window and increments
 *  the version counter.
 *  @return Version of the update.
 */
PUBLIC static inline NEEDS["atomic.h"]
void
Kmem::update_smas_window (Address addr, Pd_entry entry, bool flush)
{
  unsigned index = (addr >> PDESHIFT) & PDEMASK;

  kdir[index] = entry;

  // Increment version of small space window in master copy.
  if ( flush ) 
    {
      unsigned *version = 
         kdir + ((_smas_version_1_addr >> PDESHIFT) & PDEMASK);
      unsigned volatile old;

      do 
		  {
          old = *version;
        } while ( !smp_cas( version, old, old + 2) );
    }
}

PUBLIC static inline NEEDS[<flux/x86/seg.h>]
void
Kmem::create_gdt_entries(x86_desc *gdt)
{
  fill_descriptor(gdt + gdt_code_kernel/8, 0, 0xffffffff,
        ACC_PL_K | ACC_CODE_R | ACC_A, SZ_32);
  fill_descriptor(gdt + gdt_data_kernel/8, 0, 0xffffffff,
        ACC_PL_K | ACC_DATA_W | ACC_A, SZ_32);
  fill_descriptor(gdt + gdt_code_user/8, 0, 0xbfffffff,
        ACC_PL_U | ACC_CODE_R | ACC_A, SZ_32);
  fill_descriptor(gdt + gdt_data_user/8, 0, 0xbfffffff,
        ACC_PL_U | ACC_DATA_W | ACC_A, SZ_32);
}


/** Returns the data segment to be used.
 */
PUBLIC static inline NEEDS[<flux/x86/seg.h>]
int
Kmem::data_segment (bool kernel_mode)
{
	return (kernel_mode)?gdt_data_kernel:(gdt_data_user | SEL_PL_U);
}
