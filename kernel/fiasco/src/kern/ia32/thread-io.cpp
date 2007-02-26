INTERFACE:

IMPLEMENTATION [io]:


// 
// disassamble IO statements to compute the port address and 
// the number of ports accessed
// 

/** Compute port number and size for an IO instruction.
    @param eip address of the instruction 
    @param ts thread state with registers
    @param port return port address
    @param size return number of ports accessed
    @return true if the instruction was handled successfully
      false otherwise
*/

bool
Thread::get_ioport(Address eip, trap_state * ts, 
		     unsigned * port, unsigned * size)
{
  // handle 1 Byte IO
  switch( * reinterpret_cast<unsigned char *>(eip))
    {
    case 0xec:			// in dx, al
    case 0x6c:			// insb
    case 0xee:			// out dx, al
    case 0x6e:			// outb
      *size = 0;
      *port = ts->edx & 0xffff;
      return true;
    case 0xed:			// in dx, eax
    case 0x6d:			// insd
    case 0xef:			// out eax, dx
    case 0x6f:			// outd
      *size = 2;
      *port = ts->edx & 0xffff;
      if (*port +4 <= L4_fpage::IO_PORT_MAX)
	return true;
      else		   // Access beyond L4_IOPORT_MAX
	return false;
    case 0xfa:			// cli
    case 0xfb:			// sti
      *size = L4_fpage::WHOLE_IO_SPACE;
      *port = 0;
      return true;
    }
  
  // handle 2 Byte IO
  if(! (eip < Kmem::mem_user_max -1))
    return false;

  switch( * reinterpret_cast<unsigned char *>(eip))
    {
    case 0xe4:			// in imm8, al
    case 0xe6:			// out al, imm8 
      *size = 0;
      *port = * reinterpret_cast<unsigned char *>(eip + 1);
      return true;
    case 0xe5:			// in imm8, eax
    case 0xe7:			// out eax, imm8
      *size = 2;
      *port = * reinterpret_cast<unsigned char *>(eip + 1);
      if (*port +4 <= L4_fpage::IO_PORT_MAX)
	return true;
      else		   // Access beyond L4_IOPORT_MAX
	return false;

    case 0x66:			// operand size override
      switch( * reinterpret_cast<unsigned char *>(eip +1))
	{
	case 0xed:			// in dx, ax
	case 0xef:			// out ax, dx
	case 0x6d:			// insw
	case 0x6f:			// outw
	  *size = 1;
	  *port = ts->edx & 0xffff;
	  if (*port +2 <= L4_fpage::IO_PORT_MAX)
	    return true;
	  else		   // Access beyond L4_IOPORT_MAX
	    return false;
	case 0xe5:			// in imm8, ax
	case 0xe7:			// out ax,imm8
	  *size = 1;
	  *port = *reinterpret_cast<unsigned char*>(eip + 2);
	  if (*port +2 <= L4_fpage::IO_PORT_MAX)
	    return true;
	  else
	    return false;
	}

    case 0xf3:			// REP
      switch( * reinterpret_cast<unsigned char *>(eip +1))
	{
	case 0x6c:			// REP insb
	case 0x6e:			// REP outb
	  *size = 0;
	  *port = ts->edx & 0xffff;
	  return true;
	case 0x6d:			// REP insd
	case 0x6f:			// REP outd
	  *size = 2;
	  *port = ts->edx & 0xffff;
	  if(*port +4 <= L4_fpage::IO_PORT_MAX)
	    return true;
	  else		   // Access beyond L4_IOPORT_MAX
	    return false;
	}
    }

  // handle 3 Byte IO
  if(! (eip < Kmem::mem_user_max -2))
    return false;

  if((* reinterpret_cast<Unsigned16 *>(eip) == 0x66f3 ) || // sizeoverride REP
     (* reinterpret_cast<Unsigned16 *>(eip) == 0xf366 )) // REP sizeoverride
    {
      switch( * reinterpret_cast<unsigned char *>(eip +2))
	{
	case 0x6d:			// REP insw
	case 0x6f:			// REP outw
	  *size = 1;
	  *port = ts->edx & 0xffff;
	  if(*port +2 <= L4_fpage::IO_PORT_MAX)
	    return true;
	  else		   // Access beyond L4_IOPORT_MAX
	    return false;
	}
    }


  // nothing appropriate found
  return false;
}
