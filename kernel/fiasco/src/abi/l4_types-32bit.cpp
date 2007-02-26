INTERFACE:

#include "types.h"

//--- OLD STYLE -----------------------------



//--- NEW STYLE -----------------------------

EXTENSION class L4_fpage
{
public:
  
  /**
   * @brief Flex page constants.
   */
  enum {
    WHOLE_SPACE = 32, ///< Size of the whole address space.
  };

  /**
   * @brief Create a flex page from the binary representation.
   * @param w the binary representation.
   */
  L4_fpage( Mword w = 0 );

  /**
   * @brief Get the binary representation of the flex page.
   * @return this flex page in binary representation.
   */
  Mword raw() const;

private:

  Mword _raw;

  enum {
    GRANT_BIT     = 0,
    WRITE_BIT     = 1,
    SIZE_MASK     = 0x000000fc,
    SIZE_SHIFT    = 2,
    SIZE_SIZE     = 6,
    PAGE_MASK_STUPID_MACRO_WORKAROUND     = 0xfffff000,
    PAGE_SHIFT_STUPID_MACRO_WORKAROUND    = 0,
    PAGE_SIZE_STUPID_MACRO_WORKAROUND     = 32,
  };

};

IMPLEMENTATION[32bit]:

IMPLEMENT inline
Mword L4_fpage::is_valid() const
{
  return _raw;
}

IMPLEMENT inline 
L4_fpage::L4_fpage( Mword raw )
  : _raw(raw)
{}

IMPLEMENT inline
L4_fpage::L4_fpage( Mword grant, Mword write, Mword size, Mword page )
  : _raw( (grant ? (1<<GRANT_BIT) : 0)
	  | (write ? (1<<WRITE_BIT) : 0)
	  | ((size << SIZE_SHIFT) & SIZE_MASK)
	  | ((page << PAGE_SHIFT_STUPID_MACRO_WORKAROUND) & PAGE_MASK_STUPID_MACRO_WORKAROUND) )
{}

IMPLEMENT inline
Mword L4_fpage::grant() const
{
  return _raw & (1<<GRANT_BIT);
}

IMPLEMENT inline
Mword L4_fpage::write() const
{
  return _raw & (1<<WRITE_BIT);
}

IMPLEMENT inline
Mword L4_fpage::size() const
{
  return (_raw & SIZE_MASK) >> SIZE_SHIFT;
}

IMPLEMENT inline
Mword L4_fpage::page() const
{
  return (_raw & PAGE_MASK_STUPID_MACRO_WORKAROUND) >> PAGE_SHIFT_STUPID_MACRO_WORKAROUND;
}

IMPLEMENT inline
void L4_fpage::grant( Mword w )
{
  if(w)
    _raw |= (1<<GRANT_BIT);
  else
    _raw &= ~(1<<GRANT_BIT);
}

IMPLEMENT inline
void L4_fpage::write( Mword w )
{
  if(w)
    _raw |= (1<<WRITE_BIT);
  else
    _raw &= ~(1<<WRITE_BIT);
}

IMPLEMENT inline
void L4_fpage::size( Mword w )
{
  _raw = (_raw & ~SIZE_MASK) | ((w<<SIZE_SHIFT) & SIZE_MASK);
}

IMPLEMENT inline
void L4_fpage::page( Mword w )
{
  _raw = (_raw & ~PAGE_MASK_STUPID_MACRO_WORKAROUND) | ((w<<PAGE_SHIFT_STUPID_MACRO_WORKAROUND) & PAGE_MASK_STUPID_MACRO_WORKAROUND);
}

IMPLEMENT inline
Mword L4_fpage::raw() const
{
  return _raw;
}

IMPLEMENT inline
Mword L4_fpage::is_whole_space() const
{
  return (_raw >> 2) == 32;
}
