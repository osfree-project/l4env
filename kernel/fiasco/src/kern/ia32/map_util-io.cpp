/*
 * Fiasco-ia32
 * Specific code for I/O port protection
 */

IMPLEMENTATION[io]:

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
L4_msgdope
io_map(Space *from, 
	     Address fp_from_iopage,
	     Mword fp_from_size,
	     bool fp_from_grant,
	     bool fp_from_is_whole_space,
	     Space *to, 
	     Address fp_to_iopage,
	     Mword fp_to_size,
	     bool fp_to_is_iopage,
	     bool fp_to_is_whole_space)
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

  if(!fp_to_is_iopage // ordinary memory fpage on receiver
     && !fp_to_is_whole_space)
    {				// but not complete address space
      return L4_msgdope(0);		// in this case: do nothing
    }

  // if fp_from == whole address space 
  // than try to map the full IO address space
  if(fp_from_is_whole_space) {
    fp_from_iopage = 0;
    fp_from_size = L4_fpage::WHOLE_IO_SPACE;
  }

  // compute end of sender window
  Address snd_size = 
    fp_from_size < L4_fpage::WHOLE_IO_SPACE ?
    fp_from_size : (Mword)L4_fpage::WHOLE_IO_SPACE;

  Address snd_end = fp_from_iopage + (1L << snd_size);
  if(snd_end > L4_fpage::IO_PORT_MAX)
    snd_end = L4_fpage::IO_PORT_MAX;
    
  // loop variable: current sending position
  Address snd_pos = fp_from_iopage; 

  if(fp_to_is_iopage)		// valid IO page for receiver ?
    {				// need to adjust snd_pos & snd_end
      
      // snd_pos : take the max of fp_from & fp_to
      if(snd_pos < fp_to_iopage)
	snd_pos = fp_to_iopage; 

      Address rcv_win =	fp_to_size < L4_fpage::WHOLE_IO_SPACE
				? (1L << fp_to_size) 
				: (Address)L4_fpage::IO_PORT_MAX;

      // snd_end : take min of fp_from & fp_to
      if( snd_end > fp_to_iopage + rcv_win)
	snd_end = fp_to_iopage + rcv_win;
    }

  assert(snd_pos < L4_fpage::IO_PORT_MAX);

  // loop variable to hold an IO port 
  bool snd_ports;

  bool is_sigma0 = from->is_sigma0();

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
	  Space::Status status = to->io_insert(snd_pos);
	  
	  switch(status)
	    {
	    case Space::Insert_ok:

	      if (fp_from_grant)
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
  if (fp.is_iopage())
    {
      // try to unmap our own fpage
      Address size = fp.size() < L4_fpage::WHOLE_IO_SPACE
			? (1L << fp.size()) 
			: (Address)L4_fpage::IO_PORT_MAX;
      Address port = fp.iopage();
      printf("KERNEL: flush IOFP: %04x..%04x\n", port, port+size-1);
      while (size > 0)
	{
	  space->io_delete(port);
	  port++;
	  size--;
	}
    }
}

