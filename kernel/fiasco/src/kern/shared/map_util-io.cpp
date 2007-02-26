/*
 * Fiasco-ia32
 * Specific code for I/O port protection
 */

IMPLEMENTATION[{ia32,amd64}-io]:

#include "l4_types.h"
#include "assert.h"
#include "space.h"

Mapdb*
io_mapdb_instance()
{
  static const size_t io_page_sizes[] = {0x100, 1};
  static Mapdb mapdb (0x10000 / io_page_sizes[0], io_page_sizes, 2);

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
io_map(Space *from, Address fp_from_iopage, Mword fp_from_size,
       bool fp_from_grant, bool fp_from_is_whole_space,
       Space *to,   Address fp_to_iopage,   Mword fp_to_size,
       bool fp_to_is_iopage, bool fp_to_is_whole_space)
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

  // if ordinary memory fpage on receiver but not complete address space
  // then do nothing
  if(!fp_to_is_iopage && !fp_to_is_whole_space)
    return Ipc_err (0);

  // if fp_from == whole address space
  // than try to map the full IO address space
  if(fp_from_is_whole_space)
    {
      fp_from_iopage = 0;
      fp_from_size = L4_fpage::Whole_io_space;
    }

  // compute end of sender window
  Address snd_size = fp_from_size < L4_fpage::Whole_io_space 
    			? fp_from_size 
			: (Mword)L4_fpage::Whole_io_space;

  Address snd_end  = fp_from_iopage+(1L<<snd_size) < L4_fpage::Io_port_max
                    	? fp_from_iopage+(1L<<snd_size) 
			: (Address)L4_fpage::Io_port_max;
  Address snd_pos  = fp_from_iopage;

  if(fp_to_is_iopage)		// valid IO page for receiver ?
    {				// need to adjust snd_pos & snd_end
      // snd_pos : take the max of fp_from & fp_to
      if(snd_pos < fp_to_iopage)
	snd_pos = fp_to_iopage; 

      Address rcv_win =	fp_to_size < L4_fpage::Whole_io_space
				? (1L << fp_to_size) 
				: (Address)L4_fpage::Io_port_max;

      // snd_end : take min of fp_from & fp_to
      if(snd_end > fp_to_iopage + rcv_win)
	snd_end = fp_to_iopage + rcv_win;
    }

  assert(snd_pos < L4_fpage::Io_port_max);

  return map (io_mapdb_instance(), 
	      from->io_space(), from->id(), snd_pos, snd_end - snd_pos, 
	      to->io_space(), to->id(), snd_pos, 
	      fp_from_grant, 0, 0);
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
  Address port, size;

  if (fp.is_iopage())
    {
      // try to unmap our own fpage
      size = fp.size() < L4_fpage::Whole_io_space
	? (1L << fp.size()) 
	: (Address)L4_fpage::Io_port_max;

      port = fp.iopage();
    }
  else
    {
      assert (fp.is_whole_space());
      port = 0;
      size = (Address)L4_fpage::Io_port_max;;
    }

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
