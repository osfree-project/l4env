#ifndef LIBLINUX_LINUX_KERNEL_H
#define LIBLINUX_LINUX_KERNEL_H

#include_next <linux/kernel.h>

#include <l4/log/l4log.h>

static inline void dump_stack(void)
{
	LOG_Enter();
}

#endif

