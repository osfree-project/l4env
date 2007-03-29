/*
 * Fiasco-ia32
 * Specific code for I/O port protection
 */

IMPLEMENTATION[(ia32|amd64) & io]:

#include "l4_types.h"
#include "assert.h"
#include "space.h"
#include "io_space.h"

Mapdb*
io_mapdb_instance()
{
  static const size_t io_page_sizes[] = 
    {Io_space::Map_superpage_size, 0x200, Io_space::Map_page_size};
  static Mapdb mapdb (0x10000 / io_page_sizes[0], io_page_sizes, 3);

  return &mapdb;
}

/** Map the IO port region described by "fp_from" of address space "from" 
    into address space "to". IO ports can only be mapped idempotently, 
    therefore there is no offset for fp_from and only those ports are mapped 
    that lay in the intersection of fp_from and fp_to
    @param from source address space
    @param fp_from... IO flexpage descripton for IO space range
	in source IO space	
    @param to destination address space
    @param fp_to... IO flexpage description for IO space range
	in destination IO space
    @return IPC error code that describes the status of the operation
 */
Ipc_err
io_map(Space *from, L4_fpage const &fp_from, 
       Space *to, L4_fpage const &fp_to)
{
/*   printf("io_map %u -> %u "
 * 	    "snd %08x base %x size %x rcv %08x base %x size %x\n",
 * 	    (unsigned)from->space(), (unsigned)to->space(),
 * 	    fp_from.fpage, 
 * 	    fp_from.iofp.iopage, fp_from.iofp.iosize,
 * 	    fp_to.fpage, 
 * 	    fp_to.iofp.iopage, fp_to.iofp.iosize);
 *   kdb_ke("io_fpage_map 1");
 */

  Address rcv_pos, snd_pos;
  rcv_pos = Map_traits<Io_space>::get_addr(fp_to);
  snd_pos = Map_traits<Io_space>::get_addr(fp_from);
  size_t rcv_size = fp_to.size();
  size_t snd_size = fp_from.size();

  Map_traits<Io_space>::prepare_fpage(snd_pos, snd_size);
  Map_traits<Io_space>::prepare_fpage(rcv_pos, rcv_size);
  Map_traits<Io_space>::constraint(snd_pos, snd_size, rcv_pos, rcv_size,0);

  if (snd_size == 0)
    return Ipc_err(0);

  assert(snd_pos < L4_fpage::Io_port_max);

  return map (io_mapdb_instance(), 
	      from->io_space(), from->id(), snd_pos, snd_size, 
	      to->io_space(), to->id(), snd_pos, 
	      fp_from.grant(), 0, 0);
}

/** Unmap IO mappings.
    Unmap the region described by "fp" from the IO
    space "space" and/or the IO spaces the mappings have been
    mapped into. 
    XXX not implemented yet
    @param space address space that should be flushed
    @param fp    IO flexpage descriptor of IO-space range that should
                 be flushed
    @param me_too If false, only flush recursive mappings.  If true, 
                 additionally flush the region in the given address space.
    @return true if successful
*/
unsigned
io_fpage_unmap(Space *space, L4_fpage fp, bool me_too, unsigned restriction)
{
  size_t size = fp.size();
  Address port = Map_traits<Io_space>::get_addr(fp);
  Map_traits<Io_space>::prepare_fpage(port, size);
  Map_traits<Io_space>::calc_size(size);

  // Here we _would_ reset IOPL to 0 but this doesn't make much sense
  // for only one thread since this thread may have forwarded the right
  // to other threads too. Therefore we had to walk through any thread
  // of this space.
  // 
  // current()->regs()->eflags &= ~EFLAGS_IOPL;
  
  return unmap (io_mapdb_instance(), space->io_space(), space->id(), 
		restriction, port, size, me_too, Unmap_full);
}

static inline
void
save_access_attribs (Mapdb* /*mapdb*/, const Mapdb::Frame& /*mapdb_frame*/,
		     Mapping* /*mapping*/, Io_space* /*space*/, 
		     unsigned /*page_rights*/, 
		     Address /*virt*/, Address /*phys*/, Address /*size*/,
		     bool /*me_too*/)
{}
