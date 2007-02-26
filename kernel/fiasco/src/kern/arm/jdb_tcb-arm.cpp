IMPLEMENTATION [arm]:

#include "kmem_space.h"

IMPLEMENT inline NEEDS["kmem_space.h"]
bool
Jdb_tcb::is_mapped(void const* addr)
{
  return Kmem_space::kdir()->walk(const_cast<void*>(addr),0,false).valid();
}

IMPLEMENT
void Jdb_tcb::print_entry_frame_regs()
{
  Jdb_entry_frame *ef = Jdb::get_entry_frame();
  int from_user       = ef->from_user();

  printf("Registers (before debug entry from %s mode):\n"
         "[0] %08x %08x %08x %08x  %08x %08x %08x %08x\n"
         "[8] %08x %08x %08x %08x  %08x %08x %08x %s%08x\033[m\n"
         "sp = %08x\n",
         from_user ? "user" : "kernel",
	 ef->r[0], ef->r[1],ef->r[2], ef->r[3],
	 ef->r[4], ef->r[5],ef->r[6], ef->r[7],
	 ef->r[8], ef->r[9],ef->r[10], ef->r[11],
	 ef->r[12], ef->r[13],ef->r[14], Jdb::esc_iret, ef->pc,
	 ef->ksp);
}
