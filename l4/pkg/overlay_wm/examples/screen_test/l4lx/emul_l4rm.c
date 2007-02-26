/*!
 * \file   examples/l4l-reserve/snd/emul_l4rm.c
 * \brief  sample-program controlling the rtrcvclnt - l4rm emulation part.
 *
 * \date   06/19/2001
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * This file contains the rm_* emulation functions, because we cannot run
 * the region mapper with Linux (yes, we could. Somehow.).
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

#include <l4/sys/syscalls.h>
#include <l4/sys/consts.h>
#include <l4/l4rm/l4rm.h>
#include <l4/dm_mem/dm_mem.h>

#define EINVAL -1
#define ENOMEM -2

/***************************************************************************
 * l4rm stuff
 **************************************************************************/

/* we need:
	- l4rm_area_attach
	- l4rm_attach
	- l4rm_detach
	- l4rm_area_reserve_region
*/

struct vm_area
{
  void *                  addr;
  l4_size_t           size;
  l4_offs_t               offs;
  l4dm_dataspace_t          ds;
  struct vm_area * next;
};

static int fd=0;

static struct vm_area * vm_areas;


/* dummy implementation */
int l4_is_invalid_ds(l4dm_dataspace_t ds);
int l4_is_invalid_ds(l4dm_dataspace_t ds) {
	return 0;
}

static struct vm_area* reserve_virtual_area(l4_size_t size){
    struct vm_area *vm;

    if(fd==0){
	fd = open("/dev/zero",O_RDONLY);
	if(fd==0) return 0;
    }

    vm = malloc(sizeof(struct vm_area));
    if(vm==NULL) return NULL;

    vm->addr = mmap(0, size, PROT_NONE, MAP_SHARED, fd, 0);
    if(vm->addr == NULL){
	free(vm);
	return NULL;
    }
    vm->size = size;

    /* add new area to vm area list */
    vm->next = vm_areas;
    vm_areas = vm;

    /* Until we touch the pages, they are not mapped. And if we touch
       them, we get a SIGSEGV. */
    return vm;
}
static void free_virtual_area(struct vm_area *vm){
    munmap(vm->addr, vm->size);
    free(vm);
}

/**
 * \brief Find area which contains \a addr
 */
static struct vm_area *
find_virtual_area(l4_addr_t addr)
{
  struct vm_area * vm;

  vm = vm_areas;
  while ((vm != NULL) && 
	 !((addr >= (l4_addr_t)vm->addr) && 
	   (addr < ((l4_addr_t)vm->addr + vm->size))))
    vm = vm->next;

  return vm;
}

/** 
 * \brief Find area which contains \a addr and remove from vm area list
 */
static struct vm_area *
remove_virtual_area(l4_addr_t addr)
{
  struct vm_area * vm, * tmp;

  vm = vm_areas;
  tmp = NULL;
  while ((vm != NULL) && 
	 !((addr >= (l4_addr_t)vm->addr) && 
	   (addr < ((l4_addr_t)vm->addr + vm->size))))
    {
      tmp = vm;
      vm = vm->next;
    }
  
  if (vm == NULL)
    /* nothing found */
    return NULL;

  /* remove */
  if (tmp == NULL)
    /* remove first list element */
    vm_areas = vm_areas->next;
  else
    tmp->next = vm->next;

  return vm;
}

/*!\brief Attach dataspace. 
 *
 * Find an unused map region and attach dataspace area
 * (ds_offs, ds_offs + size) to that region. 
 *
 * We use get_vm_area() to find a portion of free memory. Then, we establish
 * the mapping by issuing dataspace manager calls.
 *
 * \note We ignore the flags.
 */
int
l4rm_attach(l4dm_dataspace_t * ds, l4_size_t size, l4_offs_t ds_offs,
            l4_uint32_t flags, void ** addr){
  struct vm_area *vm;
  unsigned off;
  int err;
  l4_addr_t a,fpage_addr;
  l4_addr_t fpage_size;

  /* align size */
  size = (size + L4_PAGESIZE - 1) & L4_PAGEMASK;

  /* sanity checks */
  if (l4_is_invalid_ds(*ds))
    return -EINVAL;
                
  vm = reserve_virtual_area(size);
  if(vm==NULL) return -ENOMEM;
  
  *addr = vm->addr;

  vm->ds = *ds;
  vm->offs = ds_offs;

  a = (l4_addr_t)vm->addr;
  for (off = 0; off < size; off += L4_PAGESIZE)
    {
      err = l4dm_map_pages(ds,ds_offs,L4_PAGESIZE,(a + off) & L4_PAGEMASK,
			   L4_LOG2_PAGESIZE,0,L4DM_RW,&fpage_addr,&fpage_size);
      if (err < 0)
	{
	  int i;
	  for(i = 0; i < off; i += L4_PAGESIZE)
	    {
	      l4_fpage_unmap(l4_fpage((a + i) & L4_PAGEMASK,
				      L4_LOG2_PAGESIZE,0,0),
			     L4_FP_FLUSH_PAGE); 
	    }

	  return -ENOMEM;
	}

      ds_offs+=L4_PAGESIZE;
    }
  
  return 0;
}

/*!\brief Pretend to attach a dataspace to a preallocated region.
 *
 * This function actually only calls l4rm_attach().
 */
int 
l4rm_area_attach(l4dm_dataspace_t * ds, l4_uint32_t area, l4_size_t size,
                 l4_offs_t ds_offs, l4_uint32_t flags, void ** addr)
{
  return l4rm_attach(ds, size, ds_offs, flags, addr);
}

/*!\brief Pretend to detach a dataspace from a region.
 *
 * Actually, this function flushes the pages and frees the vm-area.
 */
int
l4rm_detach(void * addr){
  struct vm_area *vm;
  int off;
  
  vm = remove_virtual_area((l4_addr_t)addr);
  if (vm == NULL)
    return -EINVAL;
  
  for(off=0; off<vm->size; off+=L4_PAGESIZE){
    l4_fpage_unmap(l4_fpage(((unsigned)vm->addr+off)&L4_PAGEMASK,
                            L4_LOG2_PAGESIZE,0,0),
               L4_FP_FLUSH_PAGE);
  }
  
  free_virtual_area(vm);
  return 0;
}

/*!\brief Pretend to reserve an area
 *
 * This function actually does nothing.
 *
 * \note This means, areas are comletely ignored!
 */
int
l4rm_area_reserve_region(l4_addr_t addr, l4_size_t size, l4_uint32_t flags,
                         l4_uint32_t * area){
  *area = 0;
  return 0;
}

int
l4rm_direct_area_reserve_region(l4_addr_t addr, l4_size_t size, 
                                l4_uint32_t flags, l4_uint32_t * area)
{
  *area = 0;
  return 0;
}

/**
 * \brief Lookup VM address
 */
int
l4rm_lookup(void * addr, l4dm_dataspace_t * ds, l4_offs_t * offset, 
	    l4_addr_t * map_addr, l4_size_t * map_size)
{
  struct vm_area * vm;
  l4_addr_t a = (l4_addr_t)addr;

  vm = find_virtual_area(a);
  if (vm == NULL)
    return -EINVAL;

  *ds = vm->ds;
  *offset = (a - (l4_addr_t)vm->addr) + vm->offs;
  *map_addr = (l4_addr_t)vm->addr;
  *map_size = vm->size;

  return 0;
}

/* more dummies */
int 
l4rm_init(int have_l4env, l4rm_vm_range_t used[], int num_used)
{
  return 0;
}

void
l4rm_service_loop(void)
{
}

void
l4rm_show_region_list(void)
{
}

