/*
 * \brief   Virtual page-table facility
 * \author  Thomas Friebel <tf13@os.inf.tu-dresden.de>
 * \author  Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * \date    2006-11-03
 */

#ifndef _ddekit_pgtab_h
#define _ddekit_pgtab_h

#include <l4/dde/ddekit/types.h>

/* FIXME Region types may be defined by pgtab users. Do we really need them
 * here? */
enum ddekit_pgtab_type
{
	PTE_TYPE_OTHER, PTE_TYPE_LARGE, PTE_TYPE_UMA, PTE_TYPE_CONTIG
};


/**
 * Set virtual->physical mapping for VM region
 *
 * \param virtual   virtual start address for region
 * \param physical  physical start address for region
 * \param pages     number of pages in region
 * \param type      pgtab type for region
 */
void ddekit_pgtab_set_region(void *virtual, ddekit_addr_t physical, int pages, int type);


/**
 * Set virtual->physical mapping for VM region given a specific size in bytes.
 *
 * Internally, DDEKit manages regions with pages. However, DDEs do not need to tangle
 * with the underlying mechanism and therefore can use this function that takes care
 * of translating a size to an amount of pages.
 */
void ddekit_pgtab_set_region_with_size(void *virt, ddekit_addr_t phys, int size, int type);


/**
 * Clear virtual->physical mapping for VM region
 *
 * \param virtual   virtual start address for region
 * \param type      pgtab type for region
 */
void ddekit_pgtab_clear_region(void *virtual, int type);

/**
 * Get physical address for virtual address
 *
 * \param virtual  virtual address
 *
 * \return physical address
 */
ddekit_addr_t ddekit_pgtab_get_physaddr(const void *virtual);

#endif
