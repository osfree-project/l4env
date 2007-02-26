INTERFACE:

#include "l4_types.h"
#include "mapdb.h"

class Space;

L4_msgdope
io_map (Space *from, 
	Address fp_from_iopage,
	Mword fp_from_size,
	bool fp_from_grant,
	bool fp_from_is_whole_space,
	Space *to, 
	Address fp_to_iopage,
	Mword fp_to_size,
	bool fp_to_is_iopage,
	bool fp_to_is_whole_space);

void
io_fpage_unmap(Space *space, L4_fpage fp, bool me_too);


IMPLEMENTATION:

//-
