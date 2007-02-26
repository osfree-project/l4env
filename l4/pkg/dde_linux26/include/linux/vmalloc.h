#ifndef _LINUX_VMALLOC_H
#define _LINUX_VMALLOC_H

#include <linux/spinlock.h>
#include <asm/page.h>		/* pgprot_t */
#include <asm/pgtable.h>	/* PAGE_KERNEL */

/* bits in vm_struct->flags */
#define VM_IOREMAP	0x00000001	/* ioremap() and friends */
#define VM_ALLOC	0x00000002	/* vmalloc() */
#define VM_MAP		0x00000004	/* vmap()ed pages */

struct vm_struct {
	void			*addr;
	unsigned long		size;
	unsigned long		flags;
	struct page		**pages;
	unsigned int		nr_pages;
	unsigned long		phys_addr;
	struct vm_struct	*next;
};

/*
 *	Highlevel APIs for driver use
 */
extern void *vmalloc(unsigned long size);
#ifndef DDE_LINUX
extern void *vmalloc_32(unsigned long size);
extern void *__vmalloc(unsigned long size, int gfp_mask, pgprot_t prot);
#else /* DDE_LINUX */
extern void *__vmalloc(unsigned long size, int gfp_mask, pgprot_t prot);
static inline void * vmalloc_32(unsigned long size)
{
    return __vmalloc(size, GFP_KERNEL, PAGE_KERNEL);
}
#endif /* DDE_LINUX */
extern void vfree(void *addr);

extern void *vmap(struct page **pages, unsigned int count,
			unsigned long flags, pgprot_t prot);
extern void vunmap(void *addr);
 
/*
 *	Lowlevel-APIs (not for driver use!)
 */
#ifndef DDE_LINUX /* For exactly this reason, we do not support it. */
extern struct vm_struct *get_vm_area(unsigned long size, unsigned long flags);
extern struct vm_struct *remove_vm_area(void *addr);
extern int map_vm_area(struct vm_struct *area, pgprot_t prot,
			struct page ***pages);
extern void unmap_vm_area(struct vm_struct *area);
#endif /* DDE_LINUX */

/*
 *	Internals.  Dont't use..
 */
extern rwlock_t vmlist_lock;
extern struct vm_struct *vmlist;

#endif /* _LINUX_VMALLOC_H */
