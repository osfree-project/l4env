
#include <flux/x86/base_trap.h>
#include <panic.h>

void trap_dump_panic(const struct trap_state *st)
{
	trap_dump(st);

	panic("terminated due to trap\n");
}

