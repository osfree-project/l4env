INTERFACE:

/*
 * This file contains the extensions for I/O flex pages on X86.
 */

EXTENSION class L4_fpage
{
public:
  /**
   * I/O port specific constants.
   */
  enum {
    Whole_io_space = 16, ///< The order used to cover the whole I/O space.
    Io_port_max    = 1L << Whole_io_space, ///< Number of available I/O ports.
  };

  /**
   * Create the given I/O flex page.
   * @param port the port address.
   * @param order the size of the flex page is 2^order.
   * @param grant if not zero the grant bit is to be set.
   */
  static L4_fpage io(Mword port, Mword order, Mword grant);

  /**
   * Get the I/O port address.
   * @return The I/O port address.
   */
  Mword iopage() const;

  /**
   * Set the I/O port address.
   * @param addr the port address.
   */
  void iopage( Mword addr );

  /**
   * Is the flex page an I/O flex page?
   * @retunrs not zero if this flex page is an I/O flex page.
   */
  Mword is_iopage() const;

  /**
   * Covers the flex page the whole I/O space.
   * @pre The is_iopage() method must return true or the 
   *      behavior is undefined.
   * @return not zero, if the flex page covers the whole I/O 
   *          space.
   */
  Mword is_whole_io_space() const;

private:
  enum {
    Iopage_mask  = 0x0ffff000,
    Iopage_shift = 12,
  };
};

INTERFACE [32bit]:
  
EXTENSION class L4_fpage
{
private:
  enum 
  {
    Io_id        = 0xf0000000,
  };
};

INTERFACE [64bit]:
  
EXTENSION class L4_fpage
{
private:
  enum 
  {
    Io_id        = 0xfffffffff0000000UL,
  };
};


//---------------------------------------------------------------------------
IMPLEMENTATION [ia32|ux|amd64]:

IMPLEMENT inline
Mword L4_fpage::is_iopage() const
{
  return (_raw & Special_fp_mask) == Io_id;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [!(ia32|ux|amd64)]:

IMPLEMENT inline
Mword L4_fpage::is_iopage() const
{
  return 0;
}

//---------------------------------------------------------------------------
IMPLEMENTATION [iofp]:

IMPLEMENT inline
void L4_fpage::iopage( Mword w )
{
  _raw = (_raw & ~Iopage_mask) | ((w << Iopage_shift) & Iopage_mask);
}

IMPLEMENT inline
Mword L4_fpage::iopage() const
{
  return (_raw & Iopage_mask) >> Iopage_shift;
}

IMPLEMENT inline
L4_fpage L4_fpage::io( Mword port, Mword size, Mword grant )
{
  return L4_fpage( (grant ? 1 : 0)
		   | ((port << Iopage_shift) & Iopage_mask)
		   | ((size << Size_shift) & Size_mask)
		   | Io_id);
}

IMPLEMENT inline
Mword L4_fpage::is_whole_io_space() const
{ return is_iopage() && (_raw >> 2) == Whole_io_space; }

