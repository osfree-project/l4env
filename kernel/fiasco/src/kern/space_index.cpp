INTERFACE:

#include "l4_types.h"

class Space;
class Space_registry;

class Space_index
{
public:
  enum
  {
    Max_space_number = L4_uid::Max_tasks
  };

private:
  // CLASS DATA
  static Space_registry* spaces;

  // DATA
  unsigned space_id;
};

IMPLEMENTATION:

#include "atomic.h"

#include <cassert>

// 
// Space_registry
// 
struct Space_registry
{
  union {
    Space *space;
    struct {
      unsigned dead : 1;
      unsigned chief : 31;
    } state;
  };

  operator Mword () { return (Mword)space; }
  
};

// constructor: initialize to "dead = 1"
PUBLIC inline 
Space_registry::Space_registry()
{ 
  state.chief = 0;
  state.dead = 1; 
} 

// 
// Space_index
// 

static Space_registry registered_spaces[Space_index::Max_space_number];
Space_registry* Space_index::spaces = registered_spaces;

PUBLIC inline explicit
Space_index::Space_index(unsigned number) // type-conversion cons.:
				              // from task number
  : space_id(number)
{ }

PUBLIC inline 
Space_index::operator unsigned ()
{ return space_id; }

PUBLIC inline NEEDS [Space_registry]
Space *
Space_index::lookup()
{ 
  if (spaces[space_id].state.dead)
    return 0;

  return spaces[space_id].space;
}

PUBLIC inline NEEDS[Space_registry::Space_registry, "atomic.h"]
bool 
Space_index::set_chief(Space_index old_chief, Space_index new_chief)
{
  Space_registry o(spaces[space_id]);

  if (! o.state.dead || o.state.chief != old_chief)
    return false;

  Space_registry n;
  n.state.chief = new_chief;

  return cas (&spaces[space_id], o, n);
}

PUBLIC inline NEEDS [Space_index::Space_index, 
		     Space_registry, <cassert>]
Space_index 
Space_index::chief() // return chief number
{
  assert (spaces[space_id].state.dead); // space does not exist

  return Space_index(spaces[space_id].state.chief);
}

PUBLIC static inline NEEDS[Space_registry::Space_registry]
bool 
Space_index::add(Space *new_space, unsigned new_number,
		 unsigned *out_chief)
{
  Space_registry o, n;

  do 
    {
      o = (spaces[new_number]);
      if (! o.state.dead)
	return false;

      n.space = new_space;
    }
  while (! cas (&spaces[new_number], o, n));

  if (out_chief)
    *out_chief = o.state.chief;

  return true;
}


PUBLIC static inline NEEDS[<cassert>, Space_registry, Space_index::aux_del]
bool 
Space_index::del(Space_index number, Space_index chief)
{
  assert(number < Max_space_number);
  assert(! spaces[number].state.dead);

  // when deleting a task, write its owner (chief) into the task register
  spaces[number].state.dead = 1;
  spaces[number].state.chief = chief;

  aux_del(number);

  return true;
}

IMPLEMENTATION [!pl0_hack]:

PUBLIC static inline
void
Space_index::aux_del(Space_index /*number*/)
{}
