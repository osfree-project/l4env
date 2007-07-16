INTERFACE:

#include "types.h"
#include "initcalls.h"

EXTENSION class Space_index
{

public:
  enum Prctrl_cmd
  {
    Prctrl_idt       = 0,
    Prctrl_transfer  = 1,
  };

private:
  static Mword      entries[Max_space_number];
  static Unsigned32 bitmask[Max_space_number /
			    (8 * sizeof(Unsigned32))];

  enum
  {
    Offset_mask   =  0x1f,
    Index_mask    = 0x7e0,
    Index_shift   =     5,
  };

};

IMPLEMENTATION:

#include "l4_types.h"
#include "static_init.h"

Mword      Space_index::entries[Space_index::Max_space_number];
Unsigned32 Space_index::bitmask[Space_index::Max_space_number /
				(8 * sizeof(Unsigned32))];


STATIC_INITIALIZE(Space_index);

PUBLIC static FIASCO_INIT
void
Space_index::init(void)
{
  /* initially only rmgr holds privileges */
  set_bit(4, 1);
}

PUBLIC static inline
Mword
Space_index::get_entry(unsigned pos)
{ return entries[pos]; }

PUBLIC static inline
void
Space_index::set_entry(Mword task_id, Mword entry)
{
  if (task_id < Space_index::Max_space_number)
    entries[task_id] = entry;
}

static
void
Space_index::set_bit(unsigned pos, unsigned val)
{
  unsigned idx, offs;

  if (pos >= Space_index::Max_space_number)
    return;

  val &= 1;

  idx  = (pos & Index_mask) >> Index_shift;
  offs =  pos & Offset_mask;
  
  bitmask[idx] = (bitmask[idx] & ~ (1 << offs)) | (val << offs);
}

static inline
bool
Space_index::get_bit(unsigned pos)
{
  unsigned idx, offs;

  if (pos >= Space_index::Max_space_number)
    return false;

  idx  = (pos & Index_mask) >> Index_shift;
  offs =  pos & Offset_mask;
  
  return bitmask[idx] & (1 << offs);
}

PUBLIC static inline NEEDS [Space_index::get_bit]
bool
Space_index::has_pl0_privilege(Mword task_id)
{
  return get_bit(task_id);
}

PUBLIC static inline
void
Space_index::grant_privilege(Mword task_id)
{
  set_bit(task_id,1);
}

PUBLIC static inline
void
Space_index::aux_del(Space_index number)
{
   /* clear privileges upon task deletion */
  set_bit(number,0);  
}
