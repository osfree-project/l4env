INTERFACE:

#include "l4_types.h"
#include "mapdb.h"
#include "std_macros.h"

class Space;

Ipc_err
io_map (Space *from, Address fp_from_iopage, Mword fp_from_size,
	bool fp_from_grant, bool fp_from_is_whole_space,
	Space *to, Address fp_to_iopage, Mword fp_to_size,
	bool fp_to_is_iopage, bool fp_to_is_whole_space);

void
io_fpage_unmap(Space *space, L4_fpage fp, bool me_too);


//---------------------------------------------------------------------------
IMPLEMENTATION[arm]:

#include "space.h"

inline NOEXPORT
void
handle_sigma0_adapter_address(Address &phys, unsigned &page_flags)
{
  if (EXPECT_FALSE ((phys < Kmem::Sdram_phys_base) ||
	            (phys >= (Kmem::Sdram_phys_base + (64 << 20)))))
    page_flags |= Space::Page_noncacheable;
  else
    page_flags |= Page::CACHEABLE;
}

/** The mapping database.
    This is the system's instance of the mapping database.
 */
Mapdb *
mapdb_instance()
{
  static Mapdb mapdb(Kmem::Sdram_phys_base, Kmem::Sdram_phys_base + (64<<20));
  return &mapdb;
}


//---------------------------------------------------------------------------
IMPLEMENTATION[ia32,ux]:

#include "space.h"

// For IPC mapping purposes, sigma0 addresses 0x40000000-0xbfffffff
// are shifted to 0x80000000-0xffffffff -- ugly but true!
inline NOEXPORT
void
handle_sigma0_adapter_address(Address &phys, unsigned &page_flags)
{
  if (EXPECT_FALSE (phys >= 0x40000000 && phys < 0xC0000000))
    {
      phys += 0x40000000;
      page_flags |= Space::Page_noncacheable;
    }
}

/** The mapping database.
    This is the system's instance of the mapping database.
 */
Mapdb *
mapdb_instance()
{
  static Mapdb mapdb (0, Kip::k()->main_memory_high());
  return &mapdb;
}


//---------------------------------------------------------------------------
IMPLEMENTATION [!lipc]:

inline NOEXPORT
void
update_lipc_kip(Space *, Address, Mword, Space *, Address, Mword, Address)
{}


//---------------------------------------------------------------------------
IMPLEMENTATION [{ia32,ux}-v2-lipc]:

#include "kip.h"

inline NOEXPORT
void
update_lipc_kip(Space *from, Address snd_start, Mword snd_size,
		Space *to,   Address rcv_start,	Mword rcv_size,
		Address offs)
{
  // the LIPC-KIP should be from sigma0
  if (!from->is_sigma0())
    return;

  // 4k only, no offset
  if (snd_size != Config::PAGE_SIZE || rcv_size != Config::PAGE_SIZE || offs)
    return;

  // check if kip
  // because we are mapping from sigma0 no virt_to_phys necessary
  if (snd_start != Kmem::virt_to_phys(Kip::k()))
     return;

  // test if we have already a registered LIPC-KIP
  // i dont think the race here isn't a problem
  if (to->get_lipc_kip_pointer() != (Address) -1)
    return;

  // set_lipc_kip_pointer updates only the value in the
  // page_dir slot, the update of the pointer page
  // will happen on the next task switch to the target task
  to->set_lipc_kip_pointer(rcv_start);

  printf("setting kip location "L4_PTR_FMT"\n", rcv_start);
}


//---------------------------------------------------------------------------
IMPLEMENTATION:

#include <cstdio>
#include <cassert>

#include "config.h"
#include "cpu.h"
#include "globals.h"
#include "kdb_ke.h"
#include "mem_unit.h"
#include "paging.h"
#include "space.h"
#include "warn.h"

Mapdb * mapdb_instance() FIASCO_CONST;

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
    @param flush 
        Controls the behaviour if there's already sth. mapped
        in the rcv page and we can't do an access rights upgrade.
        If set, flush the rcv page and do the requested mapping.
	If not set, skip the operation and give up.
    @pre page_aligned(offs)
    @return IPC error code that describes the status of the operation
 */
Ipc_err
mem_map (Space *from, Address fp_from_page, Mword fp_from_size,
	 bool fp_from_write, bool fp_from_grant,
	 Space *to, Address fp_to_page, Mword fp_to_size,
	 Address offs, bool flush = false)
{
  assert((offs & ~Config::PAGE_MASK) == 0);

  Mapdb* mapdb = mapdb_instance();

  Ipc_err condition(0);

  // compute virtual address space regions for this operation
  size_t snd_size = fp_from_size;
  size_t snd_size_mask;
  if (snd_size >= L4_fpage::Whole_space)
    {
      snd_size = Mem_layout::User_max;
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
  if (rcv_size >= L4_fpage::Whole_space)
    {
      rcv_size = Mem_layout::User_max;
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
  
  bool no_sender_page = true;

  // loop variables
  Address rcv_addr = rcv_start + (offs & ~rcv_size_mask);
  Address snd_addr = snd_start;
  bool is_sigma0   = from->is_sigma0();

  if (snd_size == 0 || rcv_size == 0)
    {
      if (Config::conservative)
	kdb_ke("fpage transfer = nop");

      return condition;
    }

  update_lipc_kip(from, snd_start,      snd_size,
                  to,   rcv_start,      rcv_size,
                  offs);

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
	 && rcv_addr < Mem_layout::User_max) // addresses OK?
    {
      // first, look up the page table entry in the sender address
      // space -- except for sigma0, which is special-cased because it
      // doesn't need to have a fully-contructed page table

      Address size;
      Space::Status status;
      Address phys, r_phys, r_size;
      unsigned page_flags, r_attribs;
      Mapping *pm = 0;
      Space *parent;
      Address parent_addr;

      if (EXPECT_FALSE (is_sigma0))
	{
	  // special-cased because we don't do ptab lookup for sigma0
	  if (EXPECT_TRUE (Cpu::have_superpages()))
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

	  handle_sigma0_adapter_address(phys, page_flags);
	}

      else
	{
	  if (snd_addr >= Mem_layout::User_max)
	    break;
	  
	  if (EXPECT_FALSE (!from->v_lookup(snd_addr & Config::PAGE_MASK,
	                                     &phys, &size, &page_flags)))
	    {
	      // have no pgtable entry in sender -- skip
	      size = Config::PAGE_SIZE;
	      goto skip;
	    }
	}
      no_sender_page = false;

      // We have a mapping in the sender's address space
      // locate the corresponding entry in the mapping data base

      // The owner of the parent mapping.  By default, this is the
      // sender task.  However, if we create a 4K submapping from
      // a 4M submapping, and the 4K mapping doesn't have the same
      // physical address (i.e., the 4K subpage starts somewhere
      // in the middle of the 4M page), then the owner of the
      // parent mapping is sigma0.
      parent = from;
      parent_addr = snd_addr;

      // If the sender page table entry is a superpage (4mb-page),
      // check if the receiver can take a superpage fallback to 4K
      // size if the receiver's utcb_area is in the receiver's window,
      // so that we can skip only the utcb area and map the other pages
      if (size == Config::SUPERPAGE_SIZE)
	{
	  if (size > snd_size           // want to send less that a superpage?
    	      || (rcv_addr & ~Config::SUPERPAGE_MASK) // rcv page not aligned?
	      || (rcv_addr + Config::SUPERPAGE_SIZE > rcv_start + rcv_size)
						      // rcv area to small?
	      || ! to->is_mappable(rcv_addr, size))
	    {
    	      // We map a 4K mapping from a 4MB mapping

	      // XXX We should check here (via some flag or a "magic"
	      // submapping) if we're allowed to do so, because we currently
	      // can only give out 4K mappings from a single 4M mapping.

	      if (fp_from_grant)
		{
		  WARN ("XXX Can't grant from 4M page (%x: "L4_PTR_FMT
		        " -> %x: "L4_PTR_FMT")",
		    	(unsigned int)from->id(), fp_from_page,
	    		(unsigned int)to->id(), fp_to_page);
    		  kdb_ke("XXX");
		  fp_from_grant = 0;
		}

	      size = Config::PAGE_SIZE;

	      Address super_offset = snd_addr & ~Config::SUPERPAGE_MASK;

	      if (super_offset)
		{
		  // Just use OR here because phys may already contain 
		  // the offset. (As is on ARM)
		  phys |= super_offset;

		  // HACK: Our MAPDB supports partial mappings from the 
		  // middle of superpages only from Sigma0 as originator.
		  parent = sigma0_space;
		  parent_addr = phys;
		}
	    }
	}

      // find an already existing page table entry in the receiver
      if (to->v_lookup(rcv_addr & Config::PAGE_MASK, &r_phys, &r_size, 
	               &r_attribs))
	{
	  // we have something mapped.  check if we can do an
	  // upgrade mapping, otherwise skip operation

	  // Don't check here if an r/w upgrade makes sense (i.e.,
	  // we pass on r/w permission, or the receiver has a r/o
	  // mapping).  We must go through v_insert() in any case
	  // because we need its return value to signal success
	  // (condition |= L4_IPC_FPAGE_MASK).

	  if (!to->is_mappable(rcv_addr, size))
	    {
	      assert(rcv_size != Config::PAGE_SIZE);
	      WARN ("mem_map skipping UTCB area (%x: "L4_PTR_FMT
		    " -> %x: "L4_PTR_FMT")",
	    	    (unsigned int)from->id(), fp_from_page,
    		    (unsigned int)to->id(), fp_to_page);
	      goto skip;
	    }

	  bool do_flush_or_skip = false;

	  if (r_size == size)
	    {
    	      if (phys != r_phys) // different pg frame
		do_flush_or_skip = true;
	    }
	  else if ((r_size == Config::SUPERPAGE_SIZE &&
		    r_phys + (rcv_addr & ~Config::SUPERPAGE_MASK) != phys) ||
		   phys + (snd_addr & ~Config::SUPERPAGE_MASK) != r_phys)
	    do_flush_or_skip = true; // different pg frame

	  if (do_flush_or_skip)
	    {
    	      // OK, we found some stuff already mapped in the
	      // target area but not suitable for a rights
	      // upgrade.  Now decide if we may flush it away or
	      // if we have to give up.

	      if (!flush)
		goto skip;

	      fpage_unmap (to, L4_fpage (r_size == Config::PAGE_SIZE
					  ? Config::PAGE_SHIFT 
				  	  : Config::SUPERPAGE_SHIFT, 
			  		  rcv_addr),
			   true, false);
	      goto nothing_mapped;
	    }

	  // OK, so we can do an r/w upgrade. See if existing
	  // mapping is a child of our's.

	  if (Mapdb::valid_address(phys))
	    {
   	      assert (phys == to->virt_to_phys_s0 ((void*)rcv_addr));
	      pm = mapdb->lookup(to->id(), rcv_addr, phys);
	      if (! pm)
		goto skip;  // someone deleted this mapping in the meantime
		  
	      pm = pm->parent();
	      if (pm->space() != parent->id() || pm->vaddr() != parent_addr)
		{
	     	  // not a child of ours
    		  mapdb->free(pm);
		  goto skip;
		}
	    }
	}
      else
	{
    nothing_mapped:
	  if (Mapdb::valid_address(phys))
	    {
	      // look up the parent mapping in the mapping data base; this
	      // also locks the mapping tree for this page frame
	      assert (phys == parent->virt_to_phys_s0 ((void*)parent_addr));
	      pm = mapdb->lookup(parent->id(), parent_addr, phys);
	      if (! pm)
		goto skip;  // someone deleted this mapping in the meantime
	    }

	  if (r_size < size) // not enough space for superpage mapping?
	    size = Config::PAGE_SIZE;
	}

      if (! fp_from_write)
	page_flags &= ~ Space::Page_writable;

      status = to->v_insert(phys, rcv_addr, size, page_flags);
      switch (status)
	{
	case Space::Insert_warn_exists:
	case Space::Insert_warn_attrib_upgrade:
	case Space::Insert_ok:

	  // XXX We do not handle the case in which we found or upgraded an
	  // existing mapping that originated from a different source page.
	  // This could lead to spurious flushes when one of the mappings is
	  // unmapped.

	  if (fp_from_grant)
	    {
	      if (Mapdb::valid_address(phys))
		mapdb->grant(pm, to->id(), rcv_addr);

	      from->v_delete(snd_addr, size);
	      // XXX We need a TLB flush here.
	    }
	  else if (status == Space::Insert_ok)
	    {
	      if (Mapdb::valid_address(phys))
		{
		  if (! mapdb->insert(pm, to->id(), rcv_addr, size, Map_mem))
		    {
	    	      // Error -- remove mapping again.
		      to->v_delete(rcv_addr, size);

		      // XXX This is not race-free as the mapping could have
		      // been used in the mean-time, but we do not care.
		      condition.error(Ipc_err::Remapfailed);
	    	      break;
    		    }
		}
	    }
	  condition.fpage_received(1);
	  break;

	case Space::Insert_err_nomem:
	  condition.error(Ipc_err::Remapfailed);
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

  skip:

      rcv_addr += size;
      if (rcv_addr >= rcv_start + rcv_size)
	rcv_addr = rcv_start; // wrap

      snd_size -= size;
      snd_addr += size;
    }

  if (EXPECT_FALSE(no_sender_page))
    {
      WARN ("no pgtab entry in sender: from [%x]: "L4_PTR_FMT
	    " size: %04lx to [%x]",
	    (unsigned int)from->id(), fp_from_page, fp_from_size,
	    (unsigned int)to->id());
    }

  return condition;
}

/** Unmap the mappings in the region described by "fp" from the address
    space "space" and/or the address spaces the mappings have been
    mapped into. 
    @param space address space that should be flushed
    @param fp    flexpage descriptor of address-space range that should
                 be flushed
    @param me_too If false, only flush recursive mappings.  If true, 
                 additionally flush the region in the given address space.
    @param only_read_only Do not flush pages, but only revoke write-access
                 privileges
    @return true if successful
*/
void
mem_fpage_unmap(Space *space, L4_fpage fp, bool me_too, bool only_read_only)
{
  Mapdb* mapdb = mapdb_instance();

  size_t size = fp.size();
  size = (size >= L4_fpage::Whole_space) 
    ? (size_t) Mem_layout::User_max 
    : (1L << size) & Config::PAGE_MASK;
  Address start = Config::backward_compatibility
    ? fp.page() & ~(size - 1)	// size-alignment
    : fp.page() & Config::PAGE_MASK;

  Address phys;
  Address phys_size;

  unsigned flush_mode = only_read_only ? Space::Page_writable : 0;
  bool need_flush = false;

  // iterate over all pages in "space"'s page table that are mapped
  // into the specified region
  for (Address address = start;
       address < start + size && address < Mem_layout::User_max;
       )
    {
      // find a matching page table entry
      if (! space->v_lookup(address, &phys, &phys_size))
	{
	  address += phys_size;
	  continue;
	}

      // do not unmap the UTCB area
      if (me_too && !space->is_mappable(address, phys_size))
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
	  WARN ("XXX can't split superpage in unmap (task %x: fp="L4_PTR_FMT")",
		(unsigned)space->id(), fp.raw());
	  kdb_ke("XXX");

	  // cont. at next superpage
	  address = (address + Config::SUPERPAGE_SIZE -1) 
			     & Config::SUPERPAGE_MASK;
	  continue;
	}

      if (!Mapdb::valid_address(phys))
	{
	  if (me_too)
	    {
	      space->v_delete(address, phys_size, flush_mode);
	      need_flush = true;
	    }

	  address += phys_size;
	  continue;
	}

      // find all mapping subtrees belonging to this page table entry
      for (Address phys_address = phys;
	   address < start + size && phys_address < phys + phys_size;
	   address += Config::PAGE_SIZE, phys_address += Config::PAGE_SIZE)
	{
	  Mapping *parent;
	  
	  // find the corresponding mapping for "address"
	  if (phys_size == Config::SUPERPAGE_SIZE
	      && (address & ~Config::SUPERPAGE_MASK))
	    {
	      Address sigma0_address = (phys_address & Config::SUPERPAGE_MASK) 
				     + (address & ~Config::SUPERPAGE_MASK);

	      parent = mapdb->lookup(sigma0_space->id(), 
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
	      parent = mapdb->lookup(space->id(), address, phys_address);
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
	      Space* child_space = Space::id_lookup (m->space());

	      child_space->v_delete(m->vaddr(), m->size(), flush_mode);
              
	      // Not so rare case that we delete mappings in our host space.
              // With small adress spaces there will be no flush on switch
              // anymore.
	      if (Config::Small_spaces && child_space == current_space())
		need_flush = true;
	    }

	  Mapping *locked = parent->space_is_sigma0() ? parent 
						      : parent->parent();

	  if (!only_read_only)
	    mapdb->flush(parent, me_too && !parent->space_is_sigma0());

	  mapdb->free(locked);
	}
    }

  if (need_flush)
    Mem_unit::tlb_flush();
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
fpage_unmap (Space *space, L4_fpage fp, bool me_too, bool only_read_only)
{
  if(Config::enable_io_protection && (fp.is_iopage() || fp.is_whole_space()))
    io_fpage_unmap(space, fp, me_too);
  
  if(!Config::enable_io_protection || !fp.is_iopage())
    mem_fpage_unmap (space, fp, me_too, only_read_only);
}

/** Flexpage mapping.
    divert to mem_map (for memory fpages) or io_map (for IO fpages)
    @param from source address space
    @param fp_from flexpage descriptor for virtual-address space range
	in source address space
    @param to destination address space
    @param fp_to flexpage descriptor for virtual-address space range
	in destination address space
    @param offs sender-specified offset into destination flexpage
    @param grant if set, grant the fpage, otherwise map
    @pre page_aligned(offs)
    @return IPC error code that describes the status of the operation
*/
inline NEEDS ["config.h"]
Ipc_err
fpage_map(Space *from, L4_fpage fp_from, Space *to,
	  L4_fpage fp_to, Address offs, bool grant)
{
  Ipc_err result(0);

  if (Config::enable_io_protection &&
      (fp_from.is_iopage() || fp_from.is_whole_space()))
    {
      result.combine(io_map(from, 
			    fp_from.iopage(),
			    fp_from.size(),
			    grant,
			    fp_from.is_whole_space(),
			    to, 
			    fp_to.iopage(),
			    fp_to.size(),
			    fp_to.is_iopage(),
			    fp_to.is_whole_space()));
    }

  if (!fp_from.is_iopage())
    {
      result.combine (mem_map (from, 
			       fp_from.page(), 
			       fp_from.size(),
			       fp_from.write(),
			       grant, to, 
			       fp_to.page(),
			       fp_to.size(), offs));
    }

  return result;
}
