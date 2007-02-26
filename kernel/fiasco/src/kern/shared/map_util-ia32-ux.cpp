IMPLEMENTATION[ia32-ux]:

#include <flux/x86/paging.h>
#include <cstdio>
#include <cassert>


#include "config.h"
#include "cpu.h"
#include "kmem.h"
#include "space.h"
#include "kdb_ke.h"
#include "globals.h"

#include "kmem.h"
#include "regdefs.h"

// utility functions for mapping flexpages from one address space into another


/** Map the region described by "fp_from" of address space "from" into
    region "fp_to" at offset "offs" of address space "to", updating the
    mapping database as we do so.
    @param from source address space
    @param fp_from_{page, size, write, grant} flexpage description for 
	virtual-address space range in source address space
    @param to destination address space
    @param fp_to_{page, size} flexpage descripton for virtual-address 
	space range in destination address space
    @param offs sender-specified offset into destination flexpage
    @pre page_aligned(offs)
    @return IPC error code that describes the status of the operation
 */
L4_msgdope mem_map(Space *from, 
		   Address fp_from_page,
		   Mword fp_from_size,
		   bool fp_from_write,
		   bool fp_from_grant,
		   Space *to, 
		   Address fp_to_page,
		   Mword fp_to_size,
		   Address offs)
{
  assert((offs & ~Config::PAGE_MASK) == 0);

  Mapdb* mapdb = mapdb_instance();

  L4_msgdope condition(0);

  // compute virtual address space regions for this operation
  size_t snd_size = fp_from_size;
  size_t snd_size_mask;
  if (snd_size >= 32)
    {
      snd_size = Kmem::mem_user_max;
      snd_size_mask = 0;
    }
  else
    {
      snd_size = (1L << snd_size) & Config::PAGE_MASK;
      snd_size_mask = ~(snd_size - 1);
    }
  Address snd_start = Config::backward_compatibility
    ? fp_from_page & snd_size_mask // size aligment
    : fp_from_page & Config::PAGE_MASK;

  if (Config::backward_compatibility)
    offs &= snd_size_mask;
  
  size_t rcv_size = fp_to_size;
  size_t rcv_size_mask;
  if (rcv_size >= 32)
    {
      rcv_size = Kmem::mem_user_max;
      rcv_size_mask = 0;
    }
  else
    {
      rcv_size = (1L << rcv_size) & Config::PAGE_MASK;
      rcv_size_mask = ~(rcv_size - 1);
    }
  Address rcv_start = Config::backward_compatibility
    ? fp_to_page & rcv_size_mask // size aligment
    : fp_to_page & Config::PAGE_MASK;
  
  // loop variables
  Address rcv_addr = rcv_start + (offs & ~rcv_size_mask);
  Address snd_addr = snd_start;

  bool is_sigma0 = from->is_sigma0();

  if (snd_size == 0 || rcv_size == 0)
    {
      if (Config::conservative)
	kdb_ke("fpage transfer = nop");

      return condition;
    }

  // We now loop through all the pages we want to send from the
  // sender's address space, looking up appropriate parent mappings in
  // the mapping data base, and entering a child mapping and a page
  // table entry for the receiver.

  // Special care is taken for 4MB page table entries we find in the
  // sender's address space: If what we will create in the receiver is
  // not a 4MB-mapping, too, we have to find the correct parent
  // mapping for the new mapping database entry: This is the sigma0
  // mapping for all addresses != the 4MB page base address.

  // verify sender and receiver virtual addresses are still within
  // bounds; if not, bail out.  Sigma0 may send from any address (even
  // from an out-of-bound one)
  while (snd_size		// pages left for sending?
	 && rcv_addr < Kmem::mem_user_max // addresses OK?
	 && snd_addr < Kmem::mem_user_max)
    {
      // first, look up the page table entry in the sender address
      // space -- except for sigma0, which is special-cased because it
      // doesn't need to have a fully-contructed page table

      Address phys;
      size_t  size;
      unsigned page_flags;

      if (is_sigma0 || from->v_lookup(snd_addr, &phys, &size, &page_flags))
	{
	  // we have a mapping in the sender's address space
	  // locate the corresponding entry in the mapping data base

	  if (is_sigma0)	// special-cased because we don't do pt lookup
	    {			// for sigma0
	      if (Cpu::features() & FEAT_PSE)
		{
		  phys = snd_addr & Config::SUPERPAGE_MASK;
		  size = Config::SUPERPAGE_SIZE;
		}
	      else
		{
		  phys = snd_addr & Config::PAGE_MASK;
		  size = Config::PAGE_SIZE;
		}

	      page_flags = Space::Page_writable
			 | Space::Page_user_accessible;

	      // for IPC mapping purposes, sigma0 addresses from
	      // 0x40000000 .. 0xC0000000-1 are shifted to 0x80000000
	      // .. 0xffffffff -- ugly but true!
	      if (phys >= 0x40000000 && phys < 0xC0000000)
		{
		  phys += 0x40000000;
		  page_flags |= Space::Page_noncacheable;
		}
	    }
	  
	  // The owner of the parent mapping.  By default, this is the
	  // sender task.  However, if we create a 4K submapping from
	  // a 4M submapping, and the 4K mapping doesn't have the same
	  // physical address (i.e., the 4K subpage starts somewhere
	  // in the middle of the 4M page), then the owner of the
	  // parent mapping is sigma0.
	  Space *parent = from;
	  Address parent_addr = snd_addr;

	  // if the sender page table entry is a superpage (4mb-page),
	  // check if the receiver can take a superpage
	  if (size == Config::SUPERPAGE_SIZE)
	    {
	      if (size > snd_size // want to send less that a superpage?
		  || (rcv_addr & ~Config::SUPERPAGE_MASK) // rcv page not aligned?
		  || rcv_addr + Config::SUPERPAGE_SIZE > rcv_start + rcv_size)
				// rcv area to small?
		{
		  // we map a 4K mapping from a 4MB mapping

		  // XXX we should check here (via some flag or a
		  // "magic" submapping) if we're allowed to do so,
		  // because we currently can only give out 4K
		  // mappings from a single 4M mapping.

		  if (fp_from_grant)
		    {
		      printf("KERNEL: XXX can't grant from 4M page "
			     "(%x: %08x -> %x: %08x)\n", 
			     (unsigned int)from->space(), fp_from_page,
			     (unsigned int)to->space(),   fp_to_page);
		      kdb_ke("XXX");
		      // fp_from.grant( 0 );
		    }

		  size = Config::PAGE_SIZE;

		  Address super_offset = snd_addr & ~Config::SUPERPAGE_MASK;

		  if (super_offset)
		    {
		      phys += super_offset;
		      parent = sigma0;
		      parent_addr = phys;
		    }
		}

	    }

	  // find an already existing page table entry in the receiver
	  Address r_phys, r_size;
	  unsigned r_attribs;
	  Mapping *pm = 0;

	  if (to->v_lookup(rcv_addr, &r_phys, &r_size, &r_attribs))
	    {
	      // we have something mapped.  check if we can do an
	      // upgrade mapping, otherwise skip operation

	      // Don't check here if an r/w upgrade makes sense (i.e.,
	      // we pass on r/w permission, or the receiver has a r/o
	      // mapping).  We must go through v_insert() in any case
	      // because we need its return value to signal success
	      // (condition |= L4_IPC_FPAGE_MASK).

	      if (r_size == size)
		{
		  if (phys != r_phys) // different pg frame
		    goto skip;
		}
	      else if ((r_size == Config::SUPERPAGE_SIZE 
			&& r_phys + (rcv_addr & ~Config::SUPERPAGE_MASK) != phys)
		       || phys + (snd_addr & ~Config::SUPERPAGE_MASK) != r_phys)
		goto skip;	// different pg frame

	      // OK, so we can do an r/w upgrade.  see if existing
	      // mapping is a child of our's

#ifdef MAPDB_RAM_ONLY
	      if (phys < Kmem::info()->main_memory_high())// XXX mapdb limitation
		{
#endif
		  assert (phys == to->virt_to_phys(rcv_addr));
		  pm = mapdb->lookup(to->space(), rcv_addr, phys);

		  if (! pm)
		    goto skip;	// someone deleted this mapping in the meantime
		  
		  if (pm->parent()->space() != parent->space())
		    {
		      // not a child of ours
		      mapdb->free(pm);
		      goto skip;
		    }

		  pm = pm->parent();
#ifdef MAPDB_RAM_ONLY
		}
#endif
	    }
	  else
	    {
#ifdef MAPDB_RAM_ONLY
	      if (phys < Kmem::info()->main_memory_high())// XXX mapdb limitation
		{
#endif
		  // look up the parent mapping in the mapping data base; this
		  // also locks the mapping tree for this page frame
		  assert (phys == parent->virt_to_phys(parent_addr));
		  pm = mapdb->lookup(parent->space(), parent_addr,
				     phys);
		  
		  if (! pm)
		    goto skip;	// someone deleted this mapping in the meantime
#ifdef MAPDB_RAM_ONLY
		}
#endif

	      if (r_size < size) // not enough space for superpage mapping?
		{
		  size = Config::PAGE_SIZE;
		}
	    }


	  if (! fp_from_write)
	    page_flags &= ~ Space::Page_writable;

	  Space::Status status =
	    to->v_insert(phys, rcv_addr, size, page_flags);

	  switch (status)
	    {
	    case Space::Insert_warn_exists:
	    case Space::Insert_warn_attrib_upgrade:
	    case Space::Insert_ok:

	      // XXX We do not handle the case in which we found or
	      // upgraded an existing mapping that originated from a
	      // different source page.  This could lead to spurious
	      // flushes when one of the mappings is unmapped.

	      if (fp_from_grant)
		{
#ifdef MAPDB_RAM_ONLY		// XXX mapdb limitation
		  if (phys < Kmem::info()->main_memory_high())
		    {
#endif
		      mapdb->grant(pm, to->space(), rcv_addr);
#ifdef MAPDB_RAM_ONLY
		    }
#endif
		  from->v_delete(snd_addr, size);
		  // XXX We need a TLB flush here.
		}
	      else if (status == Space::Insert_ok)
		{
#ifdef MAPDB_RAM_ONLY		// XXX mapdb limitation
		  if (phys < Kmem::info()->main_memory_high())
		    {
#endif
		      if (mapdb->insert(pm, to->space(), 
					rcv_addr, size, Map_mem) == 0)
			{
			  // Error -- remove mapping again.  

			  // XXX This is not race-free as the mapping
			  // could have been used in the mean-time,
			  // but we do not care.
			  switch (status)
			    {
			    case Space::Insert_warn_exists:
			      break; // Do nothing -- keep original mapping.
			    case Space::Insert_warn_attrib_upgrade:
			      to->v_delete(rcv_addr, size, page_flags);
			      break;
			    case Space::Insert_ok:
			      to->v_delete(rcv_addr, size);
			      break;
			    default:
			      assert(false); // Cannot occur.
			    }

			  condition.error(L4_msgdope::REMAPFAILED);
			  break;
			}
#ifdef MAPDB_RAM_ONLY
		    }
#endif
		}
	      condition.fpage_received(1);
	      break;

	    case Space::Insert_err_nomem:
	      condition.error(L4_msgdope::REMAPFAILED);
	      break;

	    case Space::Insert_err_exists:
	      
	      if (Config::conservative)
		kdb_ke("existing mapping");

	      // Do not flag an error here -- because according to L4
	      // semantics, it isn't.
	    }

	  if (pm)
	    mapdb->free(pm);

	  if (condition.has_error())
	    return condition;
	}
      else			// have no pgtable entry in sender -- skip
	size = Config::PAGE_SIZE;
      
    skip:

      rcv_addr += size;
      if (rcv_addr >= rcv_start + rcv_size) rcv_addr = rcv_start; // wrap
      
      snd_size -= size;
      snd_addr += size;
    }

  return condition;
}

/** Unmap the mappings in the region described by "fp" from the address
    space "space" and/or the address spaces the mappings have been
    mapped into. 
    @param space address space that should be flushed
    @param fp    flexpage descriptor of address-space range that should
                 be flushed
    @param me_too If flase, only flush recursive mappings.  If false, 
                 additionally flush the region in the given address space.
    @param only_read_only Do not flush pages, but only revoke write-access
                 privileges
    @return true if successful
*/
void mem_fpage_unmap(Space *space, L4_fpage fp, bool me_too, 
		     bool only_read_only)
{
  Mapdb* mapdb = mapdb_instance();

  size_t size = fp.size();
  size = (size >= 32) ? Kmem::mem_user_max : (1L << size) & Config::PAGE_MASK;
  Address start = Config::backward_compatibility
    ? fp.page() & ~(size - 1)	// size-alignment
    : fp.page() & Config::PAGE_MASK;

  Address phys;
  size_t  phys_size;

  unsigned flush_mode = only_read_only ? Space::Page_writable : 0;
  bool need_flush = false;

  // iterate over all pages in "space"'s page table that are mapped
  // into the specified region
  for (Address address = start;
       address < start + size
	 && address < Kmem::mem_user_max;
       )
    {
      // find a matching page table entry
      if (! space->v_lookup(address, &phys, &phys_size))
	{
	  address += phys_size;
	  continue;
	}

      // XXX we can't split 4MB mappings yet
      if (phys_size == Config::SUPERPAGE_SIZE
	  && me_too
	  && (address + Config::SUPERPAGE_SIZE > start + size
	      || (address & ~Config::SUPERPAGE_MASK)))
	{
	  printf("KERNEL: XXX can't split superpage in unmap "
	         "(task %x: fp=%08x)\n",
		 (unsigned)space->space(), fp.raw());
	  kdb_ke("XXX");

	  // cont. at next superpage
	  address = (address + Config::SUPERPAGE_SIZE -1) & Config::SUPERPAGE_MASK; 
	  continue;
	}

#ifdef MAPDB_RAM_ONLY
      if (phys >= Kmem::info()->main_memory_high())
	{
	  if (me_too)
	    {
	      space->v_delete(address, phys_size, flush_mode);
	      need_flush = true;
	    }

	  address += phys_size;
	  continue;
	}
#endif

      // find all mapping subtrees belonging to this page table entry
      for (Address phys_address = phys;
	   address < start + size
	     && phys_address < phys + phys_size;
	   address += Config::PAGE_SIZE,
	     phys_address += Config::PAGE_SIZE)
	{
	  Mapping *parent;
	  
	  // find the corresponding mapping for "address"
	  if (phys_size == Config::SUPERPAGE_SIZE
	      && (address & ~Config::SUPERPAGE_MASK))
	    {
	      Address sigma0_address = 
		(phys_address & Config::SUPERPAGE_MASK) + (address & ~Config::SUPERPAGE_MASK);

	      parent = mapdb->lookup(sigma0->space(), 
				     sigma0_address, sigma0_address);
	      assert(parent);
	  
	      // first delete from this address space
	      // XXX Do we need this here?
	      if (me_too)
		{
		  space->v_delete(address, Config::PAGE_SIZE, flush_mode);
		  need_flush = true;
		}
	    }
	  else
	    {
	      parent = mapdb->lookup(space->space(), address, 
				     phys_address);
	      
	      assert(parent);
	      
	      // first delete from this address space
	      if (me_too)
		{
		  space->v_delete(address, phys_size, flush_mode);
		  need_flush = true;
		}
	    }

	  // now delete from the other address spaces
	  for (Mapping *m = parent->next_child(parent);
	       m;
	       m = m->next_child(parent))
	    {
	      Space* child_space = m->space_ptr();

	      child_space->v_delete(m->vaddr(), m->size(), flush_mode);
              
	      //Not so rare case that we delete mappings in our host space.
              //With small adress spaces there will be no flush on switch
              //anymore.
	      if ( Config::USE_SMALL_SPACES )
	      {
                if (child_space == current_space() ) 
		{
                  need_flush = true;
                }
	      }
	      
	    }

	  Mapping *locked = 
	    // NOTE: Don't use parent->space_ptr()->is_sigma0() here.
	    // Sigma0 may already be deleted from space index.
	    parent->space_is_sigma0()
	    ? parent : parent->parent();

	  if (!only_read_only)
	    {
	      mapdb->flush(parent, me_too && !parent->space_is_sigma0());
	    }

	  mapdb->free(locked);

	}
    }

  if (need_flush)
    Kmem::tlb_flush();

}




/** The mapping database.
    This is the system's instance of the mapping database.
 */
Mapdb *mapdb_instance()
{
  static Mapdb mapdb (0, Kmem::info()->main_memory_high());
  return &mapdb;
}



/** Flexpage unmapping.
    divert to mem_fpage_unmap (for memory fpages) or 
    io_fpage_unmap (for IO fpages)
    @param space address space that should be flushed
    @param fp    flexpage descriptor of address-space range that should
                 be flushed
    @param me_too If flase, only flush recursive mappings.  If false, 
                 additionally flush the region in the given address space.
    @param only_read_only Do not flush pages, but only revoke write-access
                 privileges
    @return true if successful
*/
INLINE NEEDS ["config.h"]
void
fpage_unmap(Space *space, L4_fpage fp, bool me_too, bool only_read_only)
{
  if(Config::enable_io_protection &&
     (fp.is_iopage() // an IO flex page ?? or everything ??
      || fp.is_whole_space()))
    {
      io_fpage_unmap(space, fp, me_too);
    }
  
  if(!Config::enable_io_protection || !fp.is_iopage())
    {
      mem_fpage_unmap(space, fp, me_too, only_read_only);
    }
}

