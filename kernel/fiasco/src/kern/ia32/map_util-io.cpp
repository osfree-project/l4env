/*
 * Fiasco-ia32
 * Specific code for I/O port protection
 */

IMPLEMENTATION[io]:

#include "context.h"
#include "entry_frame.h"

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
  Ipc_err condition(0);
  
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

  // loop variable to hold an IO port 
  bool snd_ports;
  bool is_sigma0 = from->is_sigma0();

  // Loop now and do the mapping
  for (; snd_pos < snd_end; snd_pos++)
    {
      snd_ports = is_sigma0 ? true : from->io_lookup(snd_pos);

      // do something if sender has mapped something and receiver has nothing
      if (snd_ports && ! to->io_lookup(snd_pos))
	{
	  switch (to->io_insert(snd_pos))
	    {
	    case Space::Insert_ok:
	      if (fp_from_grant)
      		from->io_delete(snd_pos);
	      condition.fpage_received(1);
	      break;

	    case Space::Insert_err_nomem:
	      condition.error(Ipc_err::Remapfailed);
	      return condition;

	    default:
	      assert(false);
	    }
	}
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
  if (fp.is_iopage())
    {
      // try to unmap our own fpage
      Address size = fp.size() < L4_fpage::Whole_io_space
			? (1L << fp.size()) 
			: (Address)L4_fpage::Io_port_max;
      Address port = fp.iopage();

      // Here we _would_ reset IOPL to 0 but this doesn't make much sense
      // for only one thread since this thread may have forwarded the right
      // to other threads too. Therefore we had to walk through any thread
      // of this space.
      // 
      // current()->regs()->eflags &= ~EFLAGS_IOPL;

      while (size > 0 && port < L4_fpage::Io_port_max)
	{
	  space->io_delete(port);
	  port++;
	  size--;
	}
    }
}

