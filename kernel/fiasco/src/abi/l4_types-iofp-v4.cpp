INTERFACE:

EXTENSION class L4_fpage
{
public:
  /**
   * @brief Create the given I/O flex page.
   * @param port the port address.
   * @param order the size of the flex page is 2^order.
   * @param read if true the read bit is to be set.
   * @param write if true the write bit is to be set.
   * @param exec if true the exec bit is to be set.
   */
  static L4_fpage io (Mword port, Mword order, 
		      bool read, bool write, bool exec);

  /**
   * @brief v2/x0 compatibility IO fpage constructor
   */
  static L4_fpage io (Mword port, Mword order, 
		      bool grant_dummy);

private:
  enum {
    IO_ID_MASK		= 0x000003f0,
    IO_SIZE_MASK	= 0x0000fc00,
    IO_PAGE_MASK	= 0xffff0000,
    IO_EXEC_SHIFT	= 0,
    IO_WRITE_SHIFT	= 1,
    IO_READ_SHIFT	= 2,
    IO_ID_SHIFT		= 4,
    IO_ID		= 0x00000020,
    IO_SIZE_SHIFT	= 10,
    IO_PAGE_SHIFT	= 16,
  };
};

IMPLEMENTATION[iofp-v4]:

IMPLEMENT inline
Mword L4_fpage::iopage() const
{
  return (_raw & IO_PAGE_MASK) >> IO_PAGE_SHIFT;
}

IMPLEMENT inline
void L4_fpage::iopage( Mword w )
{
  _raw = (_raw & ~IO_PAGE_MASK) | ((w << IO_PAGE_SHIFT) & IO_PAGE_MASK);
}

IMPLEMENT inline
Mword L4_fpage::is_iopage() const
{
  return (_raw & IO_ID_MASK) == IO_ID;
}

IMPLEMENT inline
L4_fpage L4_fpage::io( Mword port, Mword size, 
		       bool read, bool write, bool exec )
{
  return L4_fpage( ((port << IO_PAGE_SHIFT) & IO_PAGE_MASK)
		   | ((size << IO_SIZE_SHIFT) & IO_SIZE_MASK)
		   | IO_ID
		   | (read ? (1 << IO_READ_SHIFT) : 0)
		   | (write ? (1 << IO_WRITE_SHIFT) : 0)
		   | (exec ? (1 << IO_EXEC_SHIFT) : 0) );
}

IMPLEMENT inline
L4_fpage L4_fpage::io( Mword port, Mword size, 
		       bool grant_dummy )
{
  return L4_fpage( ((port << IO_PAGE_SHIFT) & IO_PAGE_MASK)
		   | ((size << IO_SIZE_SHIFT) & IO_SIZE_MASK)
		   | IO_ID );
}

IMPLEMENT inline
Mword L4_fpage::is_whole_io_space() const
{
  return (_raw >> IO_SIZE_SHIFT) == WHOLE_IO_SPACE;
}

