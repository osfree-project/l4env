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

};


IMPLEMENTATION[iofp]:

//-
