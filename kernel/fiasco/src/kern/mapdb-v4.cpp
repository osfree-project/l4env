INTERFACE:

/** ABI specific description of an address space.
 * In v2 it contains a space number, and in v4 a space pointer.
 */
class Mapdb_space {
public:
  Space * value;
};

IMPLEMENTATION[v4]:

#include "globals.h"
#include "space.h"

PUBLIC inline Mapdb_space::Mapdb_space (Space* x) { value=x; }

/** Address space.
    @return the address space into which the frame is mapped. 
 */
PUBLIC inline NEEDS[Mapping::data]
Space *
Mapping::space()
{
  return data()->space();
}

/** Address space pointer.
    Returns the address space pointer (not: number) for all ABIs.
    The way how to get it is ABI specific.
    @return the address space into which the frame is mapped. 
 */
PUBLIC inline NEEDS[Mapping::data]
Space *
Mapping::space_ptr()
{
  return data()->space();
}

PUBLIC inline NEEDS["globals.h"]
bool
Mapping::space_is_sigma0()
{
  return space() == sigma0;
}
