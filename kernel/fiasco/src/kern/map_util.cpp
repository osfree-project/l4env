INTERFACE:

#include "l4_types.h"
#include "space.h"

class Mapdb;

/** Unmap arguments and results.  Elements can be bit-ORed together. */
enum Unmap_flags 
{
  Unmap_r = Mem_space::Page_user_accessible, ///< Revoke (at least) read access
  Unmap_w = Mem_space::Page_writable,        ///< Revoke write access
  Unmap_referenced = Mem_space::Page_referenced, ///< Reset referenced bit 
  Unmap_dirty = Mem_space::Page_dirty,       ///< Reset dirty bit 

  Unmap_none = 0,
  Unmap_full = Unmap_r | Unmap_w,

  Unmap_error = ~ unsigned(Unmap_full),   ///< An error occured while unmapping
};

IMPLEMENTATION:

#include <cassert>

#include "config.h"
#include "paging.h"
#include "warn.h"

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
// Don't inline -- it eats too much stack.
// inline NEEDS ["config.h", io_map]
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

  if (fp_from.is_cappage() || fp_from.is_whole_space())
    {
      result.combine(cap_map(from,
			    fp_from.task(),
			    fp_from.size(),
			    grant,
			    fp_from.is_whole_space(),
			    to,
			    fp_to.task(),
			    fp_to.size(),
			    fp_to.is_cappage(),
			    fp_to.is_whole_space()));
    }

  if (!fp_from.is_iopage() && !fp_from.is_cappage())
    {
      result.combine (mem_map (from,
			       fp_from.page(),
			       fp_from.size(),
			       fp_from.write(),
			       grant,
			       to,
			       fp_to.page(),
			       fp_to.size(),
			       offs,
			       fp_from.cache_type()));
    }

  return result;
}

/** Flexpage unmapping.
    divert to mem_fpage_unmap (for memory fpages) or
    io_fpage_unmap (for IO fpages)
    @param space address space that should be flushed
    @param fp    flexpage descriptor of address-space range that should
                 be flushed
    @param me_too If false, only flush recursive mappings.  If true,
                 additionally flush the region in the given address space.
    @param flush_mode determines which access privileges to remove.
    @return combined (bit-ORed) access status of unmapped physical pages
*/
// Don't inline -- it eats too much stack.
// inline NEEDS ["config.h", io_fpage_unmap]
unsigned
fpage_unmap (Space *space, L4_fpage fp, bool me_too, unsigned restriction,
	     unsigned flush_mode)
{
  unsigned ret = 0;

  if (Config::enable_io_protection && (fp.is_iopage() || fp.is_whole_space()))
    ret |= io_fpage_unmap(space, fp, me_too, restriction);
  
  if (fp.is_cappage() || fp.is_whole_space())
    ret |= cap_fpage_unmap(space, fp, me_too, restriction);
  
  if (! (Config::enable_io_protection && fp.is_iopage())
      && ! fp.is_cappage())
    ret |= mem_fpage_unmap (space, fp, me_too, restriction, flush_mode);

  return ret;
}

//////////////////////////////////////////////////////////////////////
// 
// Utility functions for all address-space types
// 

#include "mapdb.h"

template <typename SPACE>
Ipc_err
map (Mapdb* mapdb,
     SPACE* from, unsigned from_id, Address snd_addr, Mword snd_size, 
     SPACE* to, unsigned to_id, Address rcv_addr, 
     bool grant, unsigned attrib_add, unsigned attrib_del)
{
  enum { 
    PAGE_SIZE = SPACE::Map_page_size,
    PAGE_MASK = ~(PAGE_SIZE - 1),
    SUPERPAGE_SIZE = SPACE::Map_superpage_size,
    SUPERPAGE_MASK = ~(SUPERPAGE_SIZE - 1)
  };

  Ipc_err condition(0);

  bool no_page_mapped = true;
  const Address rcv_start = rcv_addr;
  const Address rcv_size = snd_size;

  // We now loop through all the pages we want to send from the
  // sender's address space, looking up appropriate parent mappings in
  // the mapping data base, and entering a child mapping and a page
  // table entry for the receiver.

  // Special care is taken for 4MB page table entries we find in the
  // sender's address space: If what we will create in the receiver is
  // not a 4MB-mapping, too, we have to find the correct parent
  // mapping for the new mapping database entry: This is the sigma0
  // mapping for all addresses != the 4MB page base address.

  // When overmapping an existing page, flush the interfering
  // physical page in the receiver, even if it is larger than the
  // mapped page.

  // verify sender and receiver virtual addresses are still within
  // bounds; if not, bail out.  Sigma0 may send from any address (even
  // from an out-of-bound one)
  Address size = PAGE_SIZE;
  bool need_tlb_flush = false;

  for (;
       snd_size			// pages left for sending?
	 && rcv_addr < SPACE::Map_max_address; // addresses OK?

       rcv_addr += size,
	 snd_addr += size,
	 snd_size -= size)
    {
      // Reset the increment size to one page.
      size = PAGE_SIZE;

      // First, look up the page table entries in the sender and
      // receiver address spaces.

      // Sender lookup.
      Address s_phys, s_size;
      unsigned s_attribs;

      // Sigma0 special case: Sigma0 doesn't need to have a
      // fully-constructed page table, and it can fabricate mappings
      // for all physical addresses.
      if (EXPECT_TRUE (! from->v_fabricate (from_id, snd_addr, 
					    &s_phys, &s_size, &s_attribs)))
	{
	  if (snd_addr >= SPACE::Map_max_address)
	    break;

	  if (EXPECT_FALSE (!from->v_lookup(snd_addr & PAGE_MASK,
					    &s_phys, &s_size, &s_attribs)))
	    {
	      // have no pgtable entry in sender -- skip
	      continue;
	    }
	}

      // We have a mapping in the sender's address space.
      no_page_mapped = false;

      // Receiver lookup.  
      Address r_phys, r_size;
      unsigned r_attribs;

      // Also, look up mapping database entry.  Depending on whether
      // we can overmap, either look up the destination mapping first
      // (and compute the sender mapping from it) or look up the
      // sender mapping directly.
      Mapping* sender_mapping = 0;
      Mapdb::Frame mapdb_frame;
      bool doing_upgrade = false;

      if (to->v_lookup(rcv_addr, &r_phys, &r_size, &r_attribs))
	{
	  // We have something mapped.

	  // See if we can overmap it.
	  if (!to->is_mappable(rcv_addr, PAGE_SIZE))
	    {
	      WARN ("map skipping area (%x: "L4_PTR_FMT
		    " -> %x: "L4_PTR_FMT")",
		    from_id, snd_addr,
		    to_id, rcv_addr);

	      continue;
	    }
	      
	  // Check if we can upgrade mapping.  Otherwise, flush target
	  // mapping.
	  Mapping* receiver_mapping;

	  if (! grant	    		      // Grant currently always flushes
	      && r_size <= s_size             // Rcv frame in snd frame
	      && (r_phys & ~(s_size - 1)) == s_phys
	      && mapdb->valid_address(r_phys) // Can lookup in mapdb
	      && mapdb->lookup (to_id, rcv_addr, r_phys, 
				&receiver_mapping, &mapdb_frame))
	    {
	      Mapping* receiver_parent = receiver_mapping->parent();
	      if (receiver_parent->space() == from_id
		  && mapdb->vaddr(mapdb_frame, receiver_parent) == snd_addr)
		{
		  sender_mapping = receiver_parent;
		  doing_upgrade = true;
		}
	      else		// Not my child -- cannot upgrade
		{
		  mapdb->free (mapdb_frame);
		}
	    }

	  if (! sender_mapping)	// Need flush
	    {
	      unmap (mapdb, to, to_id, 0, rcv_addr & ~(r_size - 1), r_size,
		     true, Unmap_full);
	    }
	}

      if (! sender_mapping && mapdb->valid_address(s_phys))
	{
	  if (EXPECT_FALSE(! mapdb->lookup (from_id, 
					    snd_addr & ~(s_size - 1), s_phys,
					    &sender_mapping, &mapdb_frame)))
	    {
	      continue;		// someone deleted this mapping in the meantime
	    }
	}

      // At this point, we have a lookup for the sender frame (s_phys,
      // s_size, s_attribs), the max. size of the receiver frame
      // (r_phys), the sender_mapping, and whether a receiver mapping
      // already exists (doing_upgrade).
      
      // Compute attributes for to-be-inserted frame
      Address i_phys = s_phys, i_size = s_size;
      unsigned i_attribs = s_attribs;

      // See if we have to degrade to non-superpage mappings
      if (i_size == SUPERPAGE_SIZE)
	{
	  if (i_size > snd_size          // want to send less that a superpage?
	      || i_size > r_size         // not enough space for superpage map?
	      || (snd_addr & ~SUPERPAGE_MASK) // snd page not aligned?
	      || (rcv_addr & ~SUPERPAGE_MASK) // rcv page not aligned?
	      || (rcv_addr + SUPERPAGE_SIZE > rcv_start + rcv_size)
						      // rcv area to small?
	      // If the sender page table entry is a superpage (4mb-page),
	      // check if the receiver can take a superpage fallback to 4K
	      // size if the receiver's utcb_area is in the receiver's window,
	      // so that we can skip only the utcb area and map the other pages
	      || ! to->is_mappable(rcv_addr, i_size))
	    {
	      // We map a 4K mapping from a 4MB mapping
	      i_size = PAGE_SIZE;

	      if (Address super_offset = snd_addr & ~SUPERPAGE_MASK)
		{
		  // Just use OR here because i_phys may already contain 
		  // the offset. (As is on ARM)
		  i_phys |= super_offset;
		}

	      if (grant)
		{
		  WARN ("XXX Can't GRANT page from superpage (%x: "L4_PTR_FMT
		        " -> %x: "L4_PTR_FMT"), demoting to MAP",
			from_id, snd_addr,
			to_id, rcv_addr);
		  grant = 0;
		}
	    }
	}

      i_attribs &= ~attrib_del;
      i_attribs |= attrib_add;

      // Loop increment is size of insertion
      size = i_size;

      // Do the actual insertion.
      typename SPACE::Status status 
	= to->v_insert(i_phys, rcv_addr, i_size, i_attribs);
      
      switch (status)
	{
	case SPACE::Insert_warn_exists:
	case SPACE::Insert_warn_attrib_upgrade:
	case SPACE::Insert_ok:

	  assert (mapdb->valid_address(s_phys) || status == SPACE::Insert_ok);
	  	// Never doing upgrades for mapdb-unmanaged memory

	  if (grant)
	    {
	      if (mapdb->valid_address(s_phys))
		mapdb->grant(mapdb_frame, sender_mapping, to_id, rcv_addr);

	      from->v_delete(snd_addr & ~(s_size - 1), s_size);
	      need_tlb_flush = true;
	    }
	  else if (status == SPACE::Insert_ok)
	    {
	      assert (! doing_upgrade);

	      if (mapdb->valid_address(s_phys)
		  && ! mapdb->insert(mapdb_frame, sender_mapping, 
				     to_id, rcv_addr, 
				     i_phys, i_size))
		{
		  // Error -- remove mapping again.
		  to->v_delete(rcv_addr, i_size);

		  // XXX This is not race-free as the mapping could have
		  // been used in the mean-time, but we do not care.
		  condition.error(Ipc_err::Remapfailed);
		  break;
		}
	    }
	  break;

	case SPACE::Insert_err_nomem:
	  condition.error(Ipc_err::Remapfailed);
	  break;

	case SPACE::Insert_err_exists:
	  if (Config::conservative)
	    kdb_ke("existing mapping");
	  // Do not flag an error here -- because according to L4
	  // semantics, it isn't.
	}

      if (sender_mapping)
	mapdb->free(mapdb_frame);

      if (condition.has_error())
	break;
    }

  if (need_tlb_flush)
    from->tlb_flush();

  if (EXPECT_FALSE(no_page_mapped))
    {
      WARN ("nothing mapped: from [%x]: "L4_PTR_FMT
	    " size: "L4_PTR_FMT" to [%x]",
	    from_id, snd_addr, rcv_size,
	    to_id);
    }

  return condition;
}

template <typename SPACE>
unsigned
unmap (Mapdb* mapdb, SPACE* space, unsigned space_id, unsigned restriction,
       Address start, Address size, bool me_too, 
       unsigned flush_mode)
{
  assert (! (me_too && restriction));

  unsigned flushed_rights = 0;
  Address end = start + size;  

  Address phys;
  Address phys_size;
  Address page_address;

  bool full_flush = flush_mode & Unmap_r;
  bool need_tlb_flush = false;

  // iterate over all pages in "space"'s page table that are mapped
  // into the specified region
  for (Address address = start;
       address < end 
	 && address < Mem_layout::User_max;
       address = page_address + phys_size)
    {
      address = Paging::canonize(address);
      	// XXX canonization is useful only for virtual memory.

      bool have_page;

      if (EXPECT_FALSE (space->v_fabricate (space_id, address, 
					    &phys, &phys_size)))
	{
	  have_page = true;
	  me_too = false;
	}
      else
	{
	  have_page = space->v_lookup(address, &phys, &phys_size);
	}

      page_address = address & ~(phys_size - 1);

      // phys_size and page_address have now been set up, allowing the
      // use of continue (which evaluates the for-loop's iteration
      // expression involving these to variables).

      if (! have_page)
	continue;

      // do not unmap the UTCB area
      // XXX: Why is it only skipped if "me_too" is set?  And why
      // don't we just reset the "me_too" flag?
      if (me_too && !space->is_mappable(address, phys_size))
	continue;
      
      if (me_too)
	{
	  assert (address == page_address 
		  || phys_size == SPACE::Map_superpage_size);

	  // Rewind flush address to page address.  We always flush
	  // the whole page, even if it is larger than the specified
	  // flush area.
	  address = page_address;
	  if (end < address + phys_size)
	    end = address + phys_size;
	}	  

      if (!mapdb->valid_address(phys)) // No mapdb data -> cannot recurse
	{
	  if (me_too)
	    {
	      flushed_rights |= 
		space->v_delete(address, phys_size, flush_mode);

	      need_tlb_flush = true;
	    }

	  continue;
	}

      Mapping *mapping;
      Mapdb::Frame mapdb_frame;

      if (! mapdb->lookup(space_id, page_address, phys,
			  &mapping, &mapdb_frame))
	// someone else unmapped faster
	continue;		// skip

      unsigned page_rights = 0;

      // Delete from this address space
      if (me_too)
	{
	  page_rights |= 
	    space->v_delete(address, phys_size, flush_mode);
	  need_tlb_flush = true;
	}

      // now delete from the other address spaces
      for (Mapdb_iterator m (mapdb_frame, mapping, restriction, address, end);
	   m;
	   ++m)
	{
	  SPACE* child_space;
	  check (Space::lookup_space (m->space(), &child_space));
	  
	  page_rights |= 
	    child_space->v_delete(m->page() * m.size(), m.size(), flush_mode);
	  
	  // Not so rare case that we delete mappings in our host space.
	  // With small adress spaces there will be no flush on switch
	  // anymore.
	  if (child_space->need_tlb_flush())
	    need_tlb_flush = true;
	}

      flushed_rights |= page_rights;

      // Store access attributes for later retrieval
      save_access_attribs (mapdb, mapdb_frame, mapping, 
			   space, page_rights, page_address, phys, phys_size,
			   me_too);

      if (full_flush)
	mapdb->flush(mapdb_frame, mapping, 
		     me_too && !mapping->space_is_sigma0(),
		     restriction, address, end);

      mapdb->free(mapdb_frame);
    }

  if (need_tlb_flush)
    space->tlb_flush();

  // Check for v_delete errors.  This statement might be a no-op if 
  // ~Space::Page_all_attribs == Unmap_error, but better be safe than sorry
  if (flushed_rights & ~SPACE::Page_all_attribs)
    flushed_rights |= Unmap_error;

  return flushed_rights;
}

IMPLEMENTATION[!io]:

// Empty dummy functions when I/O protection is disabled

inline
Ipc_err
io_map(Space * /*from*/, Address /*fp_from_iopage*/, Mword /*fp_from_size*/,
       bool /*fp_from_grant*/, bool /*fp_from_is_whole_space*/,
       Space * /*to*/,   Address /*fp_to_iopage*/,   Mword /*fp_to_size*/,
       bool /*fp_to_is_iopage*/, bool /*fp_to_is_whole_space*/)
{
  return Ipc_err(0);
}

inline
unsigned
io_fpage_unmap(Space * /*space*/, L4_fpage /*fp*/, bool /*me_too*/, 
	       unsigned /*restriction*/)
{
  return 0;
}

IMPLEMENTATION[!caps]:

// Empty dummy functions when task capabilities are disabled

inline
Ipc_err
cap_map(Space * /*from*/, Address /*fp_from_cappage*/, Mword /*fp_from_size*/,
       bool /*fp_from_grant*/, bool /*fp_from_is_whole_space*/,
       Space * /*to*/,   Address /*fp_to_cappage*/,   Mword /*fp_to_size*/,
       bool /*fp_to_is_cappage*/, bool /*fp_to_is_whole_space*/)
{
  return Ipc_err(0);
}

inline
unsigned
cap_fpage_unmap(Space * /*space*/, L4_fpage /*fp*/, bool /*me_too*/, 
		unsigned /*restriction*/)
{
  return 0;
}

