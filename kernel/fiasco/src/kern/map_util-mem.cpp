IMPLEMENTATION:

#include "config.h"
#include "mapdb.h"

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
    @return IPC error code that describes the status of the operation
 */
Ipc_err
mem_map (Space *from, L4_fpage const &fp_from,
         Space *to, L4_fpage const &fp_to, Address offs)
{
  // loop variables
  Address rcv_addr = Map_traits<Mem_space>::get_addr(fp_to);
  Address snd_addr = Map_traits<Mem_space>::get_addr(fp_from);

  size_t snd_size = fp_from.size();
  size_t rcv_size = fp_to.size();

  // calc size in bytes from power of tows
  Map_traits<Mem_space>::prepare_fpage(snd_addr, snd_size);
  Map_traits<Mem_space>::prepare_fpage(rcv_addr, rcv_size);
  Map_traits<Mem_space>::constraint(snd_addr, snd_size, rcv_addr, rcv_size, 
      offs);

  if (snd_size == 0)
    {
      if (Config::conservative)
	kdb_ke("fpage transfer = nop");
      return Ipc_err(0);
    }

  bool fp_from_grant = fp_from.grant();
  
  unsigned long del_attribs, add_attribs;
  Map_traits<Mem_space>::attribs(fp_from, &del_attribs, &add_attribs);

  return map (mapdb_instance(), 
	      from->mem_space(), from->id(), snd_addr, snd_size, 
	      to->mem_space(), to->id(), rcv_addr, 
	      fp_from_grant, add_attribs, del_attribs);
}

/** Unmap the mappings in the region described by "fp" from the address
    space "space" and/or the address spaces the mappings have been
    mapped into.
    @param space address space that should be flushed
    @param fp    flexpage descriptor of address-space range that should
                 be flushed
    @param me_too If false, only flush recursive mappings.  If true,
                 additionally flush the region in the given address space.
    @param flush_mode determines which access privileges to remove.
    @return combined (bit-ORed) access status of unmapped physical pages
*/
unsigned
mem_fpage_unmap(Space *space, L4_fpage fp, bool me_too, unsigned restriction,
		unsigned flush_mode)
{
  size_t size = fp.size();
  Address start = Map_traits<Mem_space>::get_addr(fp);
  Map_traits<Mem_space>::prepare_fpage(start, size);
  Map_traits<Mem_space>::calc_size(size);

  return unmap (mapdb_instance(), space->mem_space(), space->id(), 
		restriction, start, size, me_too, flush_mode);
}

static inline
void
save_access_attribs (Mapdb* mapdb, const Mapdb::Frame& mapdb_frame,
		     Mapping* mapping, Mem_space* space, unsigned page_rights, 
		     Address virt, Address phys, Address size,
		     bool me_too)
{
  if (unsigned page_accessed 
      = page_rights & (Mem_space::Page_referenced | Mem_space::Page_dirty))
    {
      Mem_space::Status status;
      Mem_space *s;
      
      // When flushing access attributes from our space as well,
      // cache them in parent space, otherwise in our space.
      if (! me_too || !mapping->parent())
	{
	  status = space->v_insert(phys, virt, size,
				   page_accessed);
	  
	  s = space;
	}
      else
	{
	  Mapping *parent = mapping->parent();
	  
	  Mem_space *parent_space 
	    = Space::id_lookup (parent->space())->mem_space();
	  Address parent_size = mapdb->size(mapdb_frame, parent);
	  Address parent_address = parent->page() * parent_size;
	  
	  status = 
	    parent_space->v_insert(phys & ~(parent_size - 1),
				   parent_address, parent_size,
				   page_accessed);
	  
	  s = parent_space;
	}
      
      assert (status == Mem_space::Insert_ok
	      || status == Mem_space::Insert_warn_exists
	      || status == Mem_space::Insert_warn_attrib_upgrade
	      || s->is_sigma0());
      // Be forgiving to sigma0 because it's address
      // space is not kept in sync with its mapping-db
      // entries.
    }
}

// 
// Mapdb instance for memory mappings
// 

Mapdb * mapdb_instance() FIASCO_CONST;

//---------------------------------------------------------------------------
IMPLEMENTATION[!64bit]:

/** The mapping database.
    This is the system's instance of the mapping database.
 */
Mapdb *
mapdb_instance()
{
  static const size_t page_sizes[] 
    = { Config::SUPERPAGE_SIZE, Config::PAGE_SIZE };

  static Mapdb mapdb (Config::Mapdb_ram_only
		        ? ((ram_end() + page_sizes[0] - 1) / page_sizes[0])
		        : 1U << (32 - Config::SUPERPAGE_SHIFT),
		      page_sizes, 2);

  return &mapdb;
}

//---------------------------------------------------------------------------
IMPLEMENTATION[amd64]:

#include "cpu.h"

/** The mapping database.
    This is the system's instance of the mapping database.
 */
Mapdb *
mapdb_instance()
{
  static const size_t amd64_page_sizes[] = 
  { Config::PML4_SIZE, Config::PDP_SIZE, 
    Config::SUPERPAGE_SIZE, Config::PAGE_SIZE };

  static const unsigned num_page_sizes = Cpu::phys_bits() >= 42 ?  4 : 3;

  static const Address largest_page_max_index = num_page_sizes == 4 ?
    Config::PML4E_MASK + 1ULL
    : 1ULL << (Cpu::phys_bits() - Config::PDPE_SHIFT);

  static const size_t* page_sizes = num_page_sizes == 4 ? 
    amd64_page_sizes : amd64_page_sizes + 1;

  static Mapdb mapdb (Config::Mapdb_ram_only
		        ? ((ram_end() + page_sizes[0] - 1) / page_sizes[0])
		        : largest_page_max_index,
		      page_sizes, num_page_sizes);
  return &mapdb;
}

//---------------------------------------------------------------------------
IMPLEMENTATION[arm]:

#include "kmem.h"
static inline Address ram_end() { return Kmem::sdram_phys_end(); }

//---------------------------------------------------------------------------
IMPLEMENTATION[!arm]:

#include "kip.h"
static inline Address ram_end() { return Kip::k()->main_memory_high(); }
