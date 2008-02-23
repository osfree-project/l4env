#ifndef LIBLINUX_LINUX_INTERRUPT_H
#define LIBLINUX_LINUX_INTERRUPT_H

#include_next <linux/interrupt.h>

extern void open_softirq(int nr, void (*action)(struct softirq_action*),
                         void *data);

#endif /* !LIBLINUX_LINUX_INTERRUPT_H */
