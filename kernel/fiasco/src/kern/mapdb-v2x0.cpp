INTERFACE:

#include "space.h"
#include "space_index.h"

/** ABI specific description of an address space.
 * In v2 it contains a space number, and in v4 a space pointer.
 */
class Mapdb_space {
public:
  unsigned value;
};

IMPLEMENTATION[v2x0]:

PUBLIC inline Mapdb_space::Mapdb_space (Space_index x) { value=x; }

/** Address space.
    @return the address space into which the frame is mapped. 
 */
PUBLIC inline NEEDS[Mapping::data]
unsigned 
Mapping::space()
{
  return data()->space();
}

/** Address space pointer.
    Returns the address space pointer (not: number) for all ABIs.
    The way how to get it is ABI specific.
    @return the address space into which the frame is mapped. 
 */
PUBLIC inline NEEDS["space_index.h", Mapping::data]
Space *
Mapping::space_ptr()
{
  return Space_index (data()->space()).lookup();
}


PUBLIC inline NEEDS["config.h"]
bool
Mapping::space_is_sigma0()
{
  return space() == Config::sigma0_taskno;
}
