typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef u32 dma_addr_t;
typedef u64 dma64_addr_t;

#include "l4/pci24/pci.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <l4/util/irq.h>

#define CONFIG_PCI_NAMES

#define printk(args...) printf(args)
#define simple_strtol	strtol

#define dev_probe_lock()		do{libpci_lock_dev_probe();}while(0)
#define dev_probe_unlock()		do{libpci_unlock_dev_probe();}while(0)
#define spin_lock_irqsave(a,b)		do{libpci_lock();(void)(b);}while(0)
#define spin_unlock_irqrestore(a,b)	do{libpci_unlock();(void)(b);}while(0)
#define read_lock(a)			libpci_lock()
#define read_unlock(a)			libpci_unlock()
#define write_lock(a)			libpci_lock()
#define write_unlock(a)			libpci_unlock()
#define cli_lock(a)			libpci_lock_cli(&(a))
#define cli_unlock(a)			libpci_unlock_cli(a)

#define __initfunc(a) a
#define __initdata
#define __devinit
#define __init
#define __setup(a,b)
#define GFP_KERNEL 0
#define GFP_ATOMIC 0
#define NR_IRQS 224
#define IO_SPACE_LIMIT	0xffff
#define PAGE_OFFSET	0
#define __va(x)		(x)

#define KERN_INFO
#define KERN_WARNING
#define KERN_ERR

#define EIO		5
#define EBUSY		16
#define EINVAL		22

#define L1_CACHE_BYTES	32

#define kmalloc(x,y) libpci_compat_kmalloc(x)
#define kfree(x) libpci_compat_kfree(x)

#define libpci_compat_MAX_DEV 32
#define libpci_compat_MAX_BUS 3
#define libpci_compat_MAX_RES 32

#define __weak __attribute__ ((weak))

extern void* libpci_compat_kmalloc(int size);
extern void libpci_compat_kfree(void*addr);
extern void libpci_udelay(int);
extern void libpci_lock(void);
extern void libpci_unlock(void);
extern void libpci_lock_cli(unsigned long *flags);
extern void libpci_unlock_cli(unsigned long flags);
extern void libpci_lock_dev_probe(void);
extern void libpci_unlock_dev_probe(void);

extern int request_resource(struct pci_resource *root,
			    struct pci_resource *new);
extern int allocate_resource(struct pci_resource *root,
			     struct pci_resource *new,
			     unsigned long size,
			     unsigned long min, unsigned long max,
			     unsigned long align,
			     void (*alignf)(void *,
					    struct pci_resource *,
					    unsigned long),
			     void *alignf_data);

