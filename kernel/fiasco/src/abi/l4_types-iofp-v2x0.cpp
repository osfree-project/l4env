INTERFACE:

EXTENSION class L4_fpage
{
public:
  /**
   * @brief Create the given I/O flex page.
   * @param port the port address.
   * @param order the size of the flex page is 2^order.
   * @param grant if not zero the grant bit is to be set.
   */
  static L4_fpage io( Mword port, Mword order, Mword grant );

private:
  enum {
    IOPAGE_MASK  = 0x0ffff000,
    IOPAGE_SHIFT = 12,
    IOID_MASK    = 0xf0000000,
    IO_ID        = 0xf0000000,
  };
};

IMPLEMENTATION[iofp-v2x0]:

IMPLEMENT inline
Mword L4_fpage::iopage() const
{
  return (_raw & IOPAGE_MASK) >> IOPAGE_SHIFT;
}

IMPLEMENT inline
void L4_fpage::iopage( Mword w )
{
  _raw = (_raw & ~IOPAGE_MASK) | ((w << IOPAGE_SHIFT) & IOPAGE_MASK);
}

IMPLEMENT inline
Mword L4_fpage::is_iopage() const
{
  return (_raw & IOID_MASK) == IO_ID;
}

IMPLEMENT inline
L4_fpage L4_fpage::io( Mword port, Mword size, Mword grant )
{
  return L4_fpage( (grant ? 1 : 0) 
		   | ((port << IOPAGE_SHIFT) & IOPAGE_MASK)
		   | ((size << SIZE_SHIFT) & SIZE_MASK)
		   | IO_ID );
}

IMPLEMENT inline
Mword L4_fpage::is_whole_io_space() const
{
  return (_raw >> 2) == WHOLE_IO_SPACE;
}

