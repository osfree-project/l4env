#ifndef __CON_HW_IOMEM_H__
#define __CON_HW_IOMEM_H__

extern int map_io_mem(l4_addr_t addr, l4_size_t size,
		      const char *id, l4_addr_t *vaddr);

#endif
