/*
 * ioport.h	Definitions of routines for detecting, reserving and
 *		allocating system resources.
 *
 * Authors:	Linus Torvalds
 */
#ifndef __PCI24_LINUX_IOPORT_H
#define __PCI24_LINUX_IOPORT_H

/*
 * Resources are tree-like, allowing
 * nesting etc..
 */
struct pci_resource {
	const char *name;
	unsigned long start, end;
	unsigned long flags;
	struct pci_resource *parent, *sibling, *child;
};

struct pci_resource_list {
	struct pci_resource_list *next;
	struct pci_resource *res;
	struct pci_dev *dev;
};

/*
 * IO resources have these defined flags.
 */
#define PCI_IORESOURCE_BITS		0x000000ff	/* Bus-specific bits */

#define PCI_IORESOURCE_IO		0x00000100	/* Resource type */
#define PCI_IORESOURCE_MEM		0x00000200
#define PCI_IORESOURCE_IRQ		0x00000400
#define PCI_IORESOURCE_DMA		0x00000800

#define PCI_IORESOURCE_PREFETCH	0x00001000	/* No side effects */
#define PCI_IORESOURCE_READONLY	0x00002000
#define PCI_IORESOURCE_CACHEABLE	0x00004000
#define PCI_IORESOURCE_RANGELENGTH	0x00008000
#define PCI_IORESOURCE_SHADOWABLE	0x00010000
#define PCI_IORESOURCE_BUS_HAS_VGA	0x00080000

#define PCI_IORESOURCE_UNSET	0x20000000
#define PCI_IORESOURCE_AUTO		0x40000000
#define PCI_IORESOURCE_BUSY		0x80000000	/* Driver has marked this resource busy */

/* ISA PnP IRQ specific bits (PCI_IORESOURCE_BITS) */
#define PCI_IORESOURCE_IRQ_HIGHEDGE		(1<<0)
#define PCI_IORESOURCE_IRQ_LOWEDGE		(1<<1)
#define PCI_IORESOURCE_IRQ_HIGHLEVEL	(1<<2)
#define PCI_IORESOURCE_IRQ_LOWLEVEL		(1<<3)

/* ISA PnP DMA specific bits (PCI_IORESOURCE_BITS) */
#define PCI_IORESOURCE_DMA_TYPE_MASK	(3<<0)
#define PCI_IORESOURCE_DMA_8BIT		(0<<0)
#define PCI_IORESOURCE_DMA_8AND16BIT	(1<<0)
#define PCI_IORESOURCE_DMA_16BIT		(2<<0)

#define PCI_IORESOURCE_DMA_MASTER		(1<<2)
#define PCI_IORESOURCE_DMA_BYTE		(1<<3)
#define PCI_IORESOURCE_DMA_WORD		(1<<4)

#define PCI_IORESOURCE_DMA_SPEED_MASK	(3<<6)
#define PCI_IORESOURCE_DMA_COMPATIBLE	(0<<6)
#define PCI_IORESOURCE_DMA_TYPEA		(1<<6)
#define PCI_IORESOURCE_DMA_TYPEB		(2<<6)
#define PCI_IORESOURCE_DMA_TYPEF		(3<<6)

/* ISA PnP memory I/O specific bits (PCI_IORESOURCE_BITS) */
#define PCI_IORESOURCE_MEM_WRITEABLE	(1<<0)	/* dup: PCI_IORESOURCE_READONLY */
#define PCI_IORESOURCE_MEM_CACHEABLE	(1<<1)	/* dup: PCI_IORESOURCE_CACHEABLE */
#define PCI_IORESOURCE_MEM_RANGELENGTH	(1<<2)	/* dup: PCI_IORESOURCE_RANGELENGTH */
#define PCI_IORESOURCE_MEM_TYPE_MASK	(3<<3)
#define PCI_IORESOURCE_MEM_8BIT		(0<<3)
#define PCI_IORESOURCE_MEM_16BIT		(1<<3)
#define PCI_IORESOURCE_MEM_8AND16BIT	(2<<3)
#define PCI_IORESOURCE_MEM_SHADOWABLE	(1<<5)	/* dup: PCI_IORESOURCE_SHADOWABLE */
#define PCI_IORESOURCE_MEM_EXPANSIONROM	(1<<6)

/* PC/ISA/whatever - the normal PC address spaces: IO and memory */
extern struct pci_resource ioport_resource;
extern struct pci_resource iomem_resource;

extern int get_resource_list(struct pci_resource *, char *buf, int size);

extern int check_resource(struct pci_resource *root, unsigned long, unsigned long);
extern int request_resource(struct pci_resource *root, struct pci_resource *new);
extern int release_resource(struct pci_resource *new);
extern int allocate_pci_resource(struct pci_resource *root, struct pci_resource *new,
			     unsigned long size,
			     unsigned long min, unsigned long max,
			     unsigned long align,
			     void (*alignf)(void *, struct pci_resource *, unsigned long),
			     void *alignf_data);

/* Convenience shorthand with allocation */
#define request_region(start,n,name)	__request_region(&ioport_resource, (start), (n), (name))
#define request_mem_region(start,n,name) __request_region(&iomem_resource, (start), (n), (name))

extern struct pci_resource * __request_region(struct pci_resource *, unsigned long start, unsigned long n, const char *name);

/* Compatibility cruft */
#define check_region(start,n)	__check_region(&ioport_resource, (start), (n))
#define release_region(start,n)	__release_region(&ioport_resource, (start), (n))
#define check_mem_region(start,n)	__check_region(&iomem_resource, (start), (n))
#define release_mem_region(start,n)	__release_region(&iomem_resource, (start), (n))

extern int __check_region(struct pci_resource *, unsigned long, unsigned long);
extern void __release_region(struct pci_resource *, unsigned long, unsigned long);

#ifndef __L4__
#define get_ioport_list(buf)	get_pci_resource_list(&ioport_resource, buf, PAGE_SIZE)
#define get_mem_list(buf)	get_pci_resource_list(&iomem_resource, buf, PAGE_SIZE)

#define HAVE_AUTOIRQ
extern void autoirq_setup(int waittime);
extern int autoirq_report(int waittime);

#endif // ndef __L4__

#endif	/* _PCI24_LINUX_IOPORT_H */
