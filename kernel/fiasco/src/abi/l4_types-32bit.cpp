INTERFACE [32bit]:

#include "types.h"

EXTENSION class L4_fpage
{
public:
  
  typedef Mword Raw;
  typedef Raw Cache_type;
  
  /**
   * Flex page constants.
   */
  enum 
  {
    Whole_space = 32, ///< Size of the whole address space.
  };

  /**
   * Create a flex page from the binary representation.
   * @param w the binary representation.
   */
  L4_fpage(Raw w = 0);

  /**
   * Get the binary representation of the flex page.
   * @return this flex page in binary representation.
   */
  Raw raw() const;

private:

  Raw _raw;

  enum 
  { 
    /* +--- 32-12 ---+ 11-10 +- 9-8 -+- 7-2 + 1 + 0 +
     * | page number |   C   | unused | size | W | G |
     * +-------------+-------+--------+------+---+---+ */
    Grant_bit        = 0, ///< G (Grant)
    Write_bit        = 1, ///< W (Write)
    Size_mask        = 0x000000fc,
    Size_shift       = 2,
    Size_size        = 6,
    Cache_type_mask  = 0x00000c00,///< C (Cache type [extension])
    Page_mask        = 0xfffff000,
    Page_shift       = 0,
    Page_size        = 32,
  };

};

//---------------------------------------------------------------------------
IMPLEMENTATION [32bit]:

IMPLEMENT inline
Mword 
L4_fpage::is_valid() const
{ return _raw; }

IMPLEMENT inline 
L4_fpage::L4_fpage(Raw raw)
  : _raw(raw)
{}

IMPLEMENT inline
L4_fpage::L4_fpage(Mword grant, Mword write, Mword size, Mword page)
  : _raw((grant ? (1<<Grant_bit) : 0)
	 | (write ? (1<<Write_bit) : 0)
	 | ((size << Size_shift) & Size_mask)
	 | ((page << Page_shift) & Page_mask))
{}

IMPLEMENT inline
L4_fpage::L4_fpage(Mword size, Mword page)
  : _raw(((size << Size_shift) & Size_mask)
	 | ((page << Page_shift) 
	    & Page_mask))
{}

IMPLEMENT inline
Mword 
L4_fpage::grant() const
{ return _raw & (1<<Grant_bit); }

IMPLEMENT inline
Mword
L4_fpage::write() const
{ return _raw & (1<<Write_bit); }

IMPLEMENT inline
Mword
L4_fpage::size() const
{ return (_raw & Size_mask) >> Size_shift; }

IMPLEMENT inline
Mword
L4_fpage::page() const
{ return (_raw & Page_mask) >> Page_shift; }

IMPLEMENT inline
void 
L4_fpage::grant(Mword w)
{
  if(w)
    _raw |= (1<<Grant_bit);
  else
    _raw &= ~(1<<Grant_bit);
}

IMPLEMENT inline
void 
L4_fpage::write(Mword w)
{
  if(w)
    _raw |= (1<<Write_bit);
  else
    _raw &= ~(1<<Write_bit);
}

IMPLEMENT inline
void 
L4_fpage::size(Mword w)
{ _raw = (_raw & ~Size_mask) | ((w<<Size_shift) & Size_mask); }

IMPLEMENT inline
void 
L4_fpage::page(Mword w)
{ _raw = (_raw & ~Page_mask) | ((w<<Page_shift) & Page_mask); }

IMPLEMENT inline
L4_fpage::Raw
L4_fpage::raw() const
{ return _raw; }

IMPLEMENT inline
Mword
L4_fpage::is_whole_space() const
{ return (_raw >> 2) == 32; }

PUBLIC inline
L4_fpage::Cache_type
L4_fpage::cache_type() const
{ return _raw & Cache_type_mask; }

