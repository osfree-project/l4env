#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/brlock.h>

rwlock_t __brlock_array[NR_CPUS][__BR_END] = 
	{ [0 ... NR_CPUS-1] = { [0 ... __BR_END-1] = RW_LOCK_UNLOCKED } };
