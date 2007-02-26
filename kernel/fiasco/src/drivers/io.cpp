INTERFACE:

#include "types.h"

/**
 * @brief PC I/O port API.
 */
class Io
{
public:
  
  /// Delay for slow I/O operations.
  static void iodelay();

  /**
   * @brief Read byte port.
   */
  static Unsigned8  in8 ( unsigned port );

  /**
   * @brief Read 16-bit port.
   */
  static Unsigned16 in16( unsigned port );

  /**
   * @brief Read 32-bit port.
   */
  static Unsigned32 in32( unsigned port );

  /**
   * @brief Write byte port.
   */
  static void out8 ( Unsigned8  val, unsigned port );

  /**
   * @brief Write 16-bit port.
   */
  static void out16( Unsigned16 val, unsigned port );

  /**
   * @brief Write 32-bit port.
   */
  static void out32( Unsigned32 val, unsigned port );

  /// @name Delayed versions.
  //@{ 

  /**
   * @brief Read 8-bit port.
   */
  static Unsigned8  in8_p ( unsigned port );

  /**
   * @brief Read 16-bit port.
   */
  static Unsigned16 in16_p( unsigned port );

  /**
   * @brief Read 32-bit port.
   */
  static Unsigned32 in32_p( unsigned port );

  /**
   * @brief Write 8-bit port.
   */
  static void out8_p ( Unsigned8  val, unsigned port );

  /**
   * @brief Write 16-bit port.
   */
  static void out16_p( Unsigned16 val, unsigned port );

  /**
   * @brief Write 32-bit port.
   */
  static void out32_p( Unsigned32 val, unsigned port );
  //@}

};


IMPLEMENTATION:

IMPLEMENT inline 
Unsigned8  Io::in8_p ( unsigned port )
{
  Unsigned8 tmp = in8(port);
  iodelay();
  return tmp;
}

IMPLEMENT inline
Unsigned16 Io::in16_p( unsigned port )
{
  Unsigned16 tmp = in16(port);
  iodelay();
  return tmp;
}

IMPLEMENT inline
Unsigned32 Io::in32_p( unsigned port )
{
  Unsigned32 tmp = in32(port);
  iodelay();
  return tmp;
}

IMPLEMENT inline
void Io::out8_p ( Unsigned8  val, unsigned port )
{
  out8(val,port); iodelay();
}

IMPLEMENT inline
void Io::out16_p( Unsigned16 val, unsigned port )
{
  out16(val,port); iodelay();
}
IMPLEMENT inline
void Io::out32_p( Unsigned32 val, unsigned port )
{
  out32(val,port); iodelay();
}
