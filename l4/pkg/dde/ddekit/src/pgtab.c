/*
 * \brief   Virtual page-table facility
 * \author  Thomas Friebel <tf13@os.inf.tu-dresden.de>
 * \author  Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * \date    2006-11-01
 *
 * This implementation uses l4rm (especially the AVL tree and userptr) to
 * manage virt->phys mappings. Each mapping region is represented by one
 * pgtab_object that is kept in the l4rm region userptr.
 *
 * For this to work, dataspaces must be attached to l4rm regions!
 */

#include <l4/dde/ddekit/pgtab.h>
#include <l4/dde/ddekit/memory.h>
#include <l4/dde/ddekit/panic.h>

#include <l4/l4rm/l4rm.h>
#include <l4/util/macros.h>

#include "config.h"

/**
 * "Page-table" object
 */
struct pgtab_object
{
	l4_addr_t va;    /* virtual start address */
	l4_addr_t pa;    /* physical start address */

	/* FIXME reconsider the following members */
	l4_size_t size;
	unsigned  type;  /* pgtab region type */
};


/*****************************
 ** Page-table facility API **
 *****************************/

/**
 * Get physical address for virtual address
 *
 * \param virtual  virtual address
 * \return physical address or 0
 */
ddekit_addr_t ddekit_pgtab_get_physaddr(const void *virtual)
{
	/* find pgtab object */
	struct pgtab_object *p = l4rm_get_userptr(virtual);
	if (!p) {
		/* XXX this is verbose */
		LOG_Error("no virt->phys mapping for %p", virtual);
		return 0;
	}

	/* return virt->phys mapping */
	l4_size_t offset = (l4_addr_t) virtual - p->va;

	return p->pa + offset;
}


int ddekit_pgtab_get_type(const void *virtual)
{
	/* find pgtab object */
	struct pgtab_object *p = l4rm_get_userptr(virtual);
	if (!p) {
		/* XXX this is verbose */
		LOG_Error("no virt->phys mapping for %p", virtual);
		return -1;
	}

	return p->type;
}


int ddekit_pgtab_get_size(const void *virtual)
{
	/* find pgtab object */
	struct pgtab_object *p = l4rm_get_userptr(virtual);
	if (!p) {
		/* XXX this is verbose */
		LOG_Error("no virt->phys mapping for %p", virtual);
		return -1;
	}

	return p->size;
}


/**
 * Clear virtual->physical mapping for VM region
 *
 * \param virtual   virtual start address for region
 * \param type      pgtab type for region
 */
void ddekit_pgtab_clear_region(void *virtual, int type)
{
	struct pgtab_object *p;

	/* find pgtab object */
	p = (struct pgtab_object *)l4rm_get_userptr(virtual);
	if (!p) {
		/* XXX this is verbose */
		LOG_Error("no virt->phys mapping for %p", virtual);
		return;
	}

	/* reset userptr in region map */
	/* XXX no error handling here */
	l4rm_set_userptr(virtual, 0);

	/* free pgtab object */
	ddekit_simple_free(p);
}


/**
 * Set virtual->physical mapping for VM region
 *
 * \param virtual   virtual start address for region
 * \param physical  physical start address for region
 * \param pages     number of pages in region
 * \param type      pgtab type for region
 */
void ddekit_pgtab_set_region(void *virtual, ddekit_addr_t physical, int pages, int type)
{
	/* allocate pgtab object */
	struct pgtab_object *p = ddekit_simple_malloc(sizeof(*p));
	if (!p) {
		LOG_Error("ddekit heap exhausted");
		return;
	}

	/* initialize pgtab object */
	p->va   = l4_trunc_page(virtual);
	p->pa   = l4_trunc_page(physical);
	p->size = pages * L4_PAGESIZE;
	p->type = type;

	/* set userptr in region map to pgtab object */
	int err = l4rm_set_userptr((void *)p->va, p);
	if (err) {
		LOG_Error("l4rm_set_userptr returned %d", err);
		ddekit_panic("l4rm_set_userptr");
		ddekit_simple_free(p);
	}
}

void ddekit_pgtab_set_region_with_size(void *virt, ddekit_addr_t phys, int size, int type)
{
	int p = l4_round_page(size);
	p >>= L4_PAGESHIFT;
	ddekit_pgtab_set_region(virt, phys, p, type);
}

