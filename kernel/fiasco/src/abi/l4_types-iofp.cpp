INTERFACE:

/*
 * This file contains the extensions for I/O flex pages on X86.
 */

EXTENSION class L4_fpage
{
public:
  /**
   * @brief I/O port specific constants.
   */
  enum {
    WHOLE_IO_SPACE = 16, ///< The order used to cover the whole I/O space.
    IO_PORT_MAX    = 1L << WHOLE_IO_SPACE, ///< The number of available I/O ports.
  };
  
  /**
   * @brief Get the I/O port address.
   * @return The I/O port address.
   */
  Mword iopage() const;

  /**
   * @brief Set the I/O port address.
   * @param addr the port address.
   */
  void iopage( Mword addr );

  /**
   * @brief Is the flex page an I/O flex page?
   * @retunrs not zero if this flex page is an I/O flex page.
   */
  Mword is_iopage() const;

  /**
   * @brief Covers the flex page the whole I/O space.
   * @pre The is_iopage() method must return true or the 
   *      behavior is undefined.
   * @return not zero, if the flex page covers the whole I/O 
   *          space.
   */
  Mword is_whole_io_space() const;

  /**
   * @brief Create the given I/O flex page.
   * @param port the prt address.
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


IMPLEMENTATION[iofp]:

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

