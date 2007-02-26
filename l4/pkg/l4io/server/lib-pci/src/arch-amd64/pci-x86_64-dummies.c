#include <linux/threads.h>
#include <asm/mpspec.h>
#include <string.h>

int smp_found_config = 0;
int mp_bus_id_to_pci_bus[MAX_MP_BUSSES];

void *__memcpy(void *d, void *s, size_t si)
{ return memcpy(d,s,si); }

