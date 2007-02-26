INTERFACE[ arm-sa1100 ]:

#include <sa1100.h>
#include <kmem.h>

typedef Sa1100_generic<Kmem::Timer_map_base> Sa1100;

