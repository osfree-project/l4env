#ifndef __TRAMPOLINE_H
#define __TRAMPOLINE_H

struct grub_multiboot_info;

void task_trampoline(l4_addr_t entry, struct grub_multiboot_info *mbi);
extern char _task_trampoline_end;

#endif

