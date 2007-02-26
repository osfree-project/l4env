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
#include "undef_oskit.h"

// utility functions for mapping flexpages from one address space into another




/** Map the region described by "fp_from" of address space "from" into
    region "fp_to" at offset "offs" of address space "to", updating the
    mapping database as we do so.
    @param from source address space
    @param fp_from flexpage descriptor for virtual-address space range
                   in source address space
    @param to destination address space
    @param fp_to flexpage descriptor for virtual-address space range
                 in destination address space
    @param offs sender-specified offset into destination flexpage
    @pre page_aligned(offs)
    @return IPC error code that describes the status of the operation
 */
L4_msgdope mem_fpage_map(Space *from, L4_fpage fp_from, Space *to, 
			 L4_fpage fp_to, Address offs)
{
  assert((offs & ~Config::PAGE_MASK) == 0);

  Mapdb* mapdb = mapdb_instance();

  L4_msgdope condition(0);

  // compute virtual address space regions for this operation
  size_t snd_size = fp_from.size();
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
    ? fp_from.page() & snd_size_mask // size aligment
    : fp_from.page() & Config::PAGE_MASK;

  if (Config::backward_compatibility)
    offs &= snd_size_mask;
  
  size_t rcv_size = fp_to.size();
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
    ? fp_to.page() & rcv_size_mask // size aligment
    : fp_to.page() & Config::PAGE_MASK;
  
  // loop variables
  Address rcv_addr = rcv_start + (offs & ~rcv_size_mask);
  Address snd_addr = snd_start;

  bool is_sigma0 = (from->space() == Config::sigma0_taskno);

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

      if ((is_sigma0 
	   || from->v_lookup(snd_addr, &phys, &size, &page_flags)))
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

		  if (fp_from.grant())
		    {
		      printf("KERNEL: XXX can't grant from 4M page "
			     "(%x: %08x -> %x: %08x)\n", 
			     (unsigned int)from->space(), fp_from.raw(),
			     (unsigned int)to->space(),   fp_to.raw());
		      kdb_ke("XXX");
		      fp_from.grant( 0 );
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
	      if (phys < Kmem::info()->main_memory.high)// XXX mapdb limitation
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
	      if (phys < Kmem::info()->main_memory.high)// XXX mapdb limitation
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


	  if (! fp_from.write())
	    page_flags &= ~ Space::Page_writable;

	  Space::status_t status =
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

	      if (fp_from.grant())
		{
#ifdef MAPDB_RAM_ONLY		// XXX mapdb limitation
		  if (phys < Kmem::info()->main_memory.high)
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
		  if (phys < Kmem::info()->main_memory.high)
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
      if (phys >= Kmem::info()->main_memory.high)
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
	      Space* child_space = Space_index(m->space()).lookup();

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
	    (parent->space() == Config::sigma0_taskno) 
	    ? parent : parent->parent();

	  if (!only_read_only)
	    {
	      mapdb->flush(parent, 
			   me_too && !(parent->space() 
				       == Config::sigma0_taskno));
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
  static Mapdb mapdb (0, Kmem::info()->main_memory.high);
  return &mapdb;
}


/** Flexpage mapping.
    divert to mem_fpage_map (for memory fpages) or 
    io_fpage_map (for IO fpages)
    @param from source address space
    @param fp_from flexpage descriptor for virtual-address space range
                   in source address space
    @param to destination address space
    @param fp_to flexpage descriptor for virtual-address space range
                 in destination address space
    @param offs sender-specified offset into destination flexpage
    @pre page_aligned(offs)
    @return IPC error code that describes the status of the operation
*/
inline NEEDS ["config.h"]
L4_msgdope fpage_map(Space *from, L4_fpage fp_from, Space *to, 
		     L4_fpage fp_to, vm_offset_t offs)
{
  L4_msgdope result(0);

  if(Config::enable_io_protection &&
     (fp_from.is_iopage()       // an IO flex page ?? or everything ??
      || fp_from.is_whole_space()))
    {
      result.combine(io_fpage_map(from, fp_from, to, fp_to));
    }
  
  if(!Config::enable_io_protection || !fp_from.is_iopage())
    {
      result.combine( mem_fpage_map(from, fp_from, to, fp_to, offs) );
    }
  return result;
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



/** Map the IO port region described by "fp_from" of address space "from" 
    into address space "to". IO ports can only be mapped idempotently, 
    therefore there is no offset for fp_from and only those ports are mapped 
    that lay in the intersection of fp_from and fp_to
    @param from source address space
    @param fp_from IO flexpage descriptor for IO space range
                   in source IO space
    @param to destination address space
    @param fp_to IO flexpage descriptor for IO space range
                 in destination IO space
    @return IPC error code that describes the status of the operation
 */
L4_msgdope io_fpage_map(Space *from, L4_fpage fp_from, Space *to, L4_fpage fp_to)
{
  L4_msgdope condition(0);
  
/*   printf("io_map %u -> %u "
 * 	    "snd %08x base %x size %x rcv %08x base %x size %x\n",
 * 	    (unsigned)from->space(), (unsigned)to->space(),
 * 	    fp_from.fpage, 
 * 	    fp_from.iofp.iopage, fp_from.iofp.iosize,
 * 	    fp_to.fpage, 
 * 	    fp_to.iofp.iopage, fp_to.iofp.iosize);
 *   kdb_ke("io_fpage_map 1");
 */

  if(!fp_to.is_iopage() // ordinary memory fpage on receiver
     && !fp_to.is_whole_space())
    {				// but not complete address space
      return L4_msgdope(0);		// in this case: do nothing
    }

  // if fp_from == whole address space 
  // than try to map the full IO address space
  if(fp_from.is_whole_space())
    fp_from = L4_fpage::io(0, L4_fpage::WHOLE_IO_SPACE, fp_from.grant());

  // compute end of sender window
  vm_offset_t snd_size = 
    fp_from.size() < L4_fpage::WHOLE_IO_SPACE ?
    fp_from.size() : (Mword)L4_fpage::WHOLE_IO_SPACE;

  vm_offset_t snd_end = fp_from.iopage() + (1L << snd_size);
  if(snd_end > L4_fpage::IO_PORT_MAX)
    snd_end = L4_fpage::IO_PORT_MAX;
    
  // loop variable: current sending position
  vm_offset_t snd_pos = fp_from.iopage(); 

  if(fp_to.is_iopage())	// valid IO page for receiver ?
    {				// need to adjust snd_pos & snd_end
      
      // snd_pos : take the max of fp_from & fp_to
      if(snd_pos < fp_to.iopage())
	snd_pos = fp_to.iopage(); 

      vm_offset_t rcv_win = 
	fp_to.size() < L4_fpage::WHOLE_IO_SPACE ?
	(1L << fp_to.size()) : (Mword)L4_fpage::IO_PORT_MAX;

      // snd_end : take min of fp_from & fp_to
      if( snd_end > fp_to.iopage() + rcv_win)
	snd_end = fp_to.iopage() + rcv_win;
    }

  assert(snd_pos < L4_fpage::IO_PORT_MAX);

  // loop variable to hold an IO port 
  bool snd_ports;

  bool is_sigma0 = (from->space() == Config::sigma0_taskno);

  // Loop now and do the mapping
  while(snd_pos < snd_end)
    {
      if(is_sigma0)
	snd_ports = true;
      else
	snd_ports = from->io_lookup(snd_pos);

      // do something if sender has mapped something and receiver has nothing
      if(snd_ports && ! to->io_lookup(snd_pos) )
	{
	  Space::status_t status = to->io_insert(snd_pos);
	  
	  switch(status)
	    {
	    case Space::Insert_ok:

	      if (fp_from.grant())
		{
		  from->io_delete(snd_pos);
		}
	      condition.fpage_received(1);
	      break;
	      
	    case Space::Insert_err_nomem:
	      condition.error(L4_msgdope::REMAPFAILED);
	      break;
	      
	    default:		// io_insert does not deliver anything else
	      assert(false);
	    }

	  if (condition.has_error())
	    return condition;
	}
      
      // increase loop variable
      snd_pos ++;
    }

  return condition;
}


/** Unmap IO mappings.
    Unmap the region described by "fp" from the IO
    space "space" and/or the IO spaces the mappings have been
    mapped into. 
    XXX not implemented yet
    @param space address space that should be flushed
    @param fp    IO flexpage descriptor of IO-space range that should
                 be flushed
    @param me_too If flase, only flush recursive mappings.  If false, 
                 additionally flush the region in the given address space.
    @return true if successful
*/
void
io_fpage_unmap(Space *space, L4_fpage fp, bool /*me_too*/)
{
  printf("KERNEL: XXX can't flush IO pages (task %x: fp=%08x)\n",
	 (unsigned)space->space(), fp.raw());
  kdb_ke("XXX");
}



