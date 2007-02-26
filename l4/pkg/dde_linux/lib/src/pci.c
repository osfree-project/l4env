/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/lib/src/pci.c
 * \brief  PCI Support
 *
 * \date   08/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/** \ingroup mod_common
 * \defgroup mod_pci PCI Bus/Device Support
 *
 * This module emulates the PCI subsystem inside the Linux kernel.
 *
 * Most of the services of this module are wrappers to libio functions. The
 * remainder is simple glue code. The PCI module supports up to \c PCI_DEVICES
 * devices at one virtual PCI bus.
 *
 * Services are:
 *
 * -# exploration of bus/attached devices (find)
 * -# device setup (bus mastering, enable/disable)
 * -# Power Management related functions
 * -# PCI device related resources
 * -# Hotplugging (not supported yet)
 * -# PCI memory pools (consistent DMA mappings...)
 * -# configuration space access
 * -# functions for Linux backward compatibility
 *
 * Requirements: (additionally to \ref pg_req)
 *
 * - initialized libio
 *
 * Configuration:
 *
 * - setup #PCI_DEVICES to configure number of supported PCI devices (default
 * 12)
 */

/* L4 */
#include <l4/env/errno.h>
#include <l4/generic_io/libio.h>

#include <l4/dde_linux/dde.h>

/* Linux */
#include <linux/pci.h>
#include <linux/list.h>

/* local */
#include "internal.h"
#include "__config.h"

/** \name Module variables
 * @{ */

/** PCI device structure array */
static struct pcidevs
{
  struct pci_dev linus;  /**< Linux device structure */
  l4io_pdev_t l4;        /**< l4io device handle */
} pcidevs[PCI_DEVICES];

/** list of all PCI devices (must be global) */
LIST_HEAD(pci_devices);

/** virtual PCI bus */
static struct pci_bus pcibus =
{
  name:    "LINUX DDE PCI BUS",
  number:  0,
};

/** initialization flag */
static int _initialized = 0;

/** @} */
/** Get L4IO device handle for given device
 *
 * \param  linus  Linux device
 *
 * \return l4io handle for device or 0 if not found
 */
static inline l4io_pdev_t __pci_get_handle(struct pci_dev *linus)
{
  return ((struct pcidevs*)linus)->l4;
}

/** Convert IO's pci_dev to Linux' pci_dev struct
 *
 * \param  l4io   IO device
 * \param  linus  Linux device
 *
 * \krishna don't know about `struct resource' pointers?
 */
static inline void __pci_io_to_linux(l4io_pci_dev_t *l4io,
                                     struct pci_dev *linus)
{
  int i;

  memset(linus, 0, sizeof(struct pci_dev));

  linus->devfn = l4io->devfn;
  linus->vendor = l4io->vendor;
  linus->device = l4io->device;
  linus->subsystem_vendor = l4io->sub_vendor;
  linus->subsystem_device = l4io->sub_device;
  linus->class = l4io->class;

  linus->irq = l4io->irq;
  for (i = 0; i < 12; i++)
    {
      linus->resource[i].name = linus->name;

      linus->resource[i].start = l4io->res[i].start;
      linus->resource[i].end = l4io->res[i].end;
      linus->resource[i].flags = l4io->res[i].flags;

      linus->resource[i].parent = NULL;
      linus->resource[i].sibling = NULL;
      linus->resource[i].child = NULL;
    }

  strcpy(&linus->name[0], &l4io->name[0]);
  strcpy(&linus->slot_name[0], &l4io->slot_name[0]);

  linus->bus = &pcibus;

  list_add_tail(&linus->global_list, &pci_devices);
  list_add_tail(&linus->bus_list, &pcibus.devices);
}

/** \name Exploration of bus/attached devices and drivers
 * @{ */

/** Check device against ID table
 *
 * \param ids  ID table
 * \param dev  target device
 *
 * \return matching device id
 *
 * Simple helper for device id matching check.
 */
const struct pci_device_id *pci_match_device(const struct pci_device_id *ids,
                                             const struct pci_dev *dev)
{
  while (ids->vendor || ids->subvendor || ids->class_mask)
    {
      if ((ids->vendor == PCI_ANY_ID || ids->vendor == dev->vendor) &&
          (ids->device == PCI_ANY_ID || ids->device == dev->device) &&
          (ids->subvendor == PCI_ANY_ID || ids->subvendor == dev->subsystem_vendor) &&
          (ids->subdevice == PCI_ANY_ID || ids->subdevice == dev->subsystem_device) &&
          !((ids->class ^ dev->class) & ids->class_mask))
        return ids;
      ids++;
    }
  return NULL;
}

/** Check device - driver compatibility
 *
 * \param drv  device driver structure
 * \param dev  PCI device structure
 *
 * \return 1 if driver claims device; 0 otherwise
 */
static int pci_announce_device(struct pci_driver *drv, struct pci_dev *dev)
{
  const struct pci_device_id *id;
  int ret = 0;

  if (drv->id_table)
    {
      id = pci_match_device(drv->id_table, dev);
      if (!id)
        {
          ret = 0;
          goto out;
        }
    }
  else
    id = NULL;

//      dev_probe_lock();
  if (drv->probe(dev, id) >= 0)
    {
      dev->driver = drv;
      ret = 1;
    }
//      dev_probe_unlock();
out:
  return ret;
}

/** Get PCI driver of given device
 * \ingroup mod_pci
 *
 * \param dev  device to query
 *
 * \return appropriate pci_driver structure or NULL
 */
struct pci_driver *pci_dev_driver(const struct pci_dev *dev)
{
  if (dev->driver)
    return dev->driver;
  return NULL;
}

/** Register PCI driver
 * \ingroup mod_pci
 *
 * \param drv  device driver structure
 *
 * \return number of pci devices which were claimed by the driver
 *
 * pci_module_init(struct pci_driver *drv) is used to initalize drivers. Doing
 * it this way keeps the drivers away from for_each_dev() or pci_find_device().
 *
 * pci_register/unregister_driver() are helpers for these and have to be
 * implemented.
 */
int pci_register_driver(struct pci_driver *drv)
{
  struct pci_dev *dev;
  int count = 0;

  pci_for_each_dev(dev)
  {
    if (!pci_dev_driver(dev))
      count += pci_announce_device(drv, dev);
  }
  return count;
}

/** Unregister PCI driver
 * \ingroup mod_pci
 *
 * \param drv  device driver structure
 *
 * \sa pci_register_driver()
 */
void pci_unregister_driver(struct pci_driver *drv)
{
  struct pci_dev *dev;

  pci_for_each_dev(dev)
    {
      if (dev->driver == drv)
        {
          if (drv->remove)
            drv->remove(dev);
          dev->driver = NULL;
        }
    }
}

/** Find PCI Device on vendor and device IDs
 * \ingroup mod_pci
 *
 * \param vendor  vendor id of desired device
 * \param device  device id of desired device
 * \param from    PCI device in list to start at (incremental calls)
 *
 * \return PCI device found or NULL on error
 */
struct pci_dev *pci_find_device(unsigned int vendor, unsigned int device,
                                const struct pci_dev *from)
{
  struct list_head *n = from ? from->global_list.next : pci_devices.next;

  while (n != &pci_devices)
    {
      struct pci_dev *dev = pci_dev_g(n);
      if ((vendor == PCI_ANY_ID || dev->vendor == vendor) &&
          (device == PCI_ANY_ID || dev->device == device))
        return dev;
      n = n->next;
    }

  return NULL;
}

/** Find PCI Device on vendor, subvendor, device and subdevice IDs
 * \ingroup mod_pci
 *
 * \param vendor     vendor id of desired device
 * \param device     device id of desired device
 * \param ss_vendor  subsystem vendor id of desired device
 * \param ss_device  subsystem device id of desired device
 * \param from       PCI device in list to start at (incremental calls)
 *
 * \return PCI device found or NULL on error
 */
struct pci_dev * pci_find_subsys(unsigned int vendor, unsigned int device,
                                 unsigned int ss_vendor, unsigned int ss_device,
                                 const struct pci_dev *from)
{
  struct list_head *n = from ? from->global_list.next : pci_devices.next;

  while (n != &pci_devices)
    {
      struct pci_dev *dev = pci_dev_g(n);
      if ((vendor == PCI_ANY_ID || dev->vendor == vendor) &&
          (device == PCI_ANY_ID || dev->device == device) &&
          (ss_vendor == PCI_ANY_ID || dev->subsystem_vendor == ss_vendor) &&
          (ss_device == PCI_ANY_ID || dev->subsystem_device == ss_device))
        return dev;
      n = n->next;
    }

  return NULL;
}

/** Find PCI Device on Slot
 * \ingroup mod_pci
 *
 * \param bus    target PCI bus
 * \param devfn  device and function number
 *
 * \return PCI device found or NULL on error
 */
struct pci_dev *pci_find_slot(unsigned int bus, unsigned int devfn)
{
  struct pci_dev *dev;

  pci_for_each_dev(dev)
    {
      if (dev->bus->number == bus && dev->devfn == devfn)
        return dev;
    }

  return NULL;
}

/** Find PCI Device on Class
 * \ingroup mod_pci
 *
 * \param class  class id of desired device
 * \param from   PCI device in list to start at (incremental calls)
 *
 * \return PCI device found or NULL on error
 */
struct pci_dev *pci_find_class(unsigned int class, const struct pci_dev *from)
{
  struct list_head *n = from ? from->global_list.next : pci_devices.next;

  while (n != &pci_devices)
    {
      struct pci_dev *dev = pci_dev_g(n);
      if (dev->class == class)
        return dev;
      n = n->next;
    }

  return NULL;
}

/* emulate locally like in drivers/pci.c */
int pci_find_capability (struct pci_dev *dev, int cap)
{
  return 0;
}

/** @} */
/** \name Device setup (bus mastering, enable/disable)
 * @{ */

/** Enable PCI Device
 * \ingroup mod_pci
 *
 * \param dev  target PCI device
 *
 * \return 0 on success; error code otherwise
 */
int pci_enable_device(struct pci_dev *dev)
{
  int err;
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      ERROR("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
    }

  err = l4io_pci_enable(pdev);

#if DEBUG_PCI
  if (err)
    ERROR("enabling PCI device (%d)", err);
#endif

  return err;
}

/** Disable PCI Device
 * \ingroup mod_pci
 *
 * \param dev  target PCI device
 */
void pci_disable_device(struct pci_dev *dev)
{
  int err;
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      ERROR("device %p not found", dev);
#endif
      return;
    }

  err = l4io_pci_disable(pdev);

#if DEBUG_PCI
  if (err)
    ERROR("disabling PCI device (%d)", err);
#endif
}

/** Set Busmastering for PCI Device
 * \ingroup mod_pci
 *
 * \param dev  target PCI device
 *
 * \todo Who panics if it fails?
 */
void pci_set_master(struct pci_dev *dev)
{
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      PANIC("device %p not found", dev);
#endif
      return;
    }

  l4io_pci_set_master(pdev);
}

/** @} */
/** \name Power Management related functions
 * @{ */

/** Set PM State for PCI Device
 * \ingroup mod_pci
 *
 * \param dev    target PCI device
 * \param state  PM state
 *
 * \return old PM state
 */
int pci_set_power_state(struct pci_dev *dev, int state)
{
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      ERROR("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
    }

  return l4io_pci_set_pm(pdev, state);
}

/** @} */
/** \name PCI device related resources
 *
 * \todo implementation
 * @{ */

/** @} */
/** \name Hotplugging (not supported yet)
 * @{ */

/** @} */
/** \name PCI memory pools (consistent DMA mappings...)
 *
 * Pool allocator ... wraps the pci_alloc_consistent page allocator, so
 * small blocks are easily used by drivers for bus mastering controllers.
 * This should probably be sharing the guts of the slab allocator.
 * @{ */

/** the pool */
struct pci_pool
{
  struct list_head page_list;
  spinlock_t lock;
  size_t blocks_per_page;
  size_t size;
  int flags;
  struct pci_dev *dev;
  size_t allocation;
  char name [32];
  wait_queue_head_t waitq;
};

/** cacheable header for 'allocation' bytes */
struct pci_page
{
  struct list_head page_list;
  void *vaddr;
  dma_addr_t dma;
  unsigned long bitmap [0];
};

#define POOL_TIMEOUT_JIFFIES ((100 /* msec */ * HZ) / 1000)
#define POOL_POISON_BYTE     0xa7

// #define CONFIG_PCIPOOL_DEBUG

/** Create a pool of pci consistent memory blocks, for dma.
 * \ingroup mod_pci
 *
 * \param name         name of pool, for diagnostics
 * \param pdev        pci device that will be doing the DMA
 * \param size        size of the blocks in this pool.
 * \param align       alignment requirement for blocks; must be a power of two
 * \param allocation  returned blocks won't cross this boundary (or zero)
 * \param flags       SLAB_* flags (not all are supported).
 *
 * Returns a pci allocation pool with the requested characteristics, or
 * null if one can't be created.  Given one of these pools, pci_pool_alloc()
 * may be used to allocate memory.  Such memory will all have "consistent"
 * DMA mappings, accessible by the device and its driver without using
 * cache flushing primitives.  The actual size of blocks allocated may be
 * larger than requested because of alignment.
 *
 * If allocation is nonzero, objects returned from pci_pool_alloc() won't
 * cross that size boundary.  This is useful for devices which have
 * addressing restrictions on individual DMA transfers, such as not crossing
 * boundaries of 4KBytes.
 */
struct pci_pool * pci_pool_create(const char *name, struct pci_dev *pdev,
                                  size_t size, size_t align, size_t allocation,
                                  int flags)
{
  struct pci_pool *retval;

  if (align == 0)
    align = 1;
  if (size == 0)
    return 0;
  else if (size < align)
    size = align;
  else if ((size % align) != 0)
    {
      size += align + 1;
      size &= ~(align - 1);
    }

  if (allocation == 0)
    {
      if (PAGE_SIZE < size)
        allocation = size;
      else
        allocation = PAGE_SIZE;
    // FIXME: round up for less fragmentation
    }
  else if (allocation < size)
    return 0;

  if (!(retval = kmalloc(sizeof *retval, flags)))
    return retval;

#ifdef CONFIG_PCIPOOL_DEBUG
  flags |= SLAB_POISON;
#endif

  strncpy(retval->name, name, sizeof retval->name);
  retval->name [sizeof retval->name - 1] = 0;

  retval->dev = pdev;
  INIT_LIST_HEAD(&retval->page_list);
  spin_lock_init(&retval->lock);
  retval->size = size;
  retval->flags = flags;
  retval->allocation = allocation;
  retval->blocks_per_page = allocation / size;
  init_waitqueue_head(&retval->waitq);

#ifdef CONFIG_PCIPOOL_DEBUG
  printk(KERN_DEBUG "pcipool create %s/%s size %d, %d/page(%d alloc)\n",
         pdev ? pdev->slot_name : NULL, retval->name, size,
         retval->blocks_per_page, allocation);
#endif

  return retval;
}

/** */
static struct pci_page * pool_alloc_page(struct pci_pool *pool, int mem_flags)
{
  struct pci_page *page;
  int mapsize;

  mapsize = pool->blocks_per_page;
  mapsize = (mapsize + BITS_PER_LONG - 1) / BITS_PER_LONG;
  mapsize *= sizeof(long);

  page = (struct pci_page *) kmalloc(mapsize + sizeof *page, mem_flags);
  if (!page)
    return 0;
  page->vaddr = pci_alloc_consistent(pool->dev, pool->allocation, &page->dma);
  if (page->vaddr)
    {
      memset(page->bitmap, 0xff, mapsize); // bit set == free
      if (pool->flags & SLAB_POISON)
        memset(page->vaddr, POOL_POISON_BYTE, pool->allocation);
      list_add(&page->page_list, &pool->page_list);
    }
  else
    {
      kfree(page);
      page = 0;
    }
  return page;
}


/** */
static inline int is_page_busy(int blocks, unsigned long *bitmap)
{
  while (blocks > 0)
    {
      if (*bitmap++ != ~0UL)
        return 1;
      blocks -= BITS_PER_LONG;
    }
  return 0;
}

/** */
static void pool_free_page(struct pci_pool *pool, struct pci_page *page)
{
  dma_addr_t dma = page->dma;

  if (pool->flags & SLAB_POISON)
    memset(page->vaddr, POOL_POISON_BYTE, pool->allocation);
  pci_free_consistent(pool->dev, pool->allocation, page->vaddr, dma);
  list_del(&page->page_list);
  kfree(page);
}


/** Destroy a pool of pci memory blocks.
 * \ingroup mod_pci
 *
 * \param pool  pci pool that will be destroyed
 *
 * Caller guarantees that no more memory from the pool is in use,
 * and that nothing will try to use the pool after this call.
 */
void pci_pool_destroy(struct pci_pool *pool)
{
  unsigned long flags;

#ifdef CONFIG_PCIPOOL_DEBUG
  printk(KERN_DEBUG "pcipool destroy %s/%s\n",
         pool->dev ? pool->dev->slot_name : NULL,
         pool->name);
#endif

  spin_lock_irqsave(&pool->lock, flags);
  while (!list_empty(&pool->page_list))
    {
      struct pci_page *page;
      page = list_entry(pool->page_list.next,
                        struct pci_page, page_list);
      if (is_page_busy(pool->blocks_per_page, page->bitmap))
        {
          printk(KERN_ERR "pci_pool_destroy %s/%s, %p busy\n",
                 pool->dev ? pool->dev->slot_name : NULL,
          pool->name, page->vaddr);
          /* leak the still-in-use consistent memory */
          list_del(&page->page_list);
          kfree(page);
        }
      else pool_free_page(pool, page);
    }
  spin_unlock_irqrestore(&pool->lock, flags);
  kfree(pool);
}


/** Get a block of consistent memory
 * \ingroup mod_pci
 *
 * \param pool       pci pool that will produce the block
 * \param mem_flags  SLAB_KERNEL or SLAB_ATOMIC
 * \param handle     pointer to dma address of block
 *
 * This returns the kernel virtual address of a currently unused block,
 * and reports its dma address through the handle.
 * If such a memory block can't be allocated, null is returned.
 */
void * pci_pool_alloc(struct pci_pool *pool, int mem_flags, dma_addr_t *handle)
{
  unsigned long flags;
  struct list_head *entry;
  struct pci_page *page;
  int map, block;
  size_t offset;
  void *retval;

restart:
  spin_lock_irqsave(&pool->lock, flags);
  list_for_each(entry, &pool->page_list)
    {
      int i;
      page = list_entry(entry, struct pci_page, page_list);
      /* only cachable accesses here ... */
      for (map = 0, i = 0; i < pool->blocks_per_page; i += BITS_PER_LONG, map++)
        {
          if (page->bitmap [map] == 0)
            continue;
          block = ffz(~ page->bitmap [map]);
          if ((i + block) < pool->blocks_per_page)
            {
              clear_bit(block, &page->bitmap [map]);
              offset = (BITS_PER_LONG * map) + block;
              offset *= pool->size;
              goto ready;
            }
        }
    }
  if (!(page = pool_alloc_page(pool, mem_flags)))
    {
      if (mem_flags == SLAB_KERNEL)
        {
          DECLARE_WAITQUEUE(wait, current);

          current->state = TASK_INTERRUPTIBLE;
          add_wait_queue(&pool->waitq, &wait);
          spin_unlock_irqrestore(&pool->lock, flags);

          schedule_timeout(POOL_TIMEOUT_JIFFIES);

          current->state = TASK_RUNNING;
          remove_wait_queue(&pool->waitq, &wait);
          goto restart;
        }
      retval = 0;
      goto done;
    }

  clear_bit(0, &page->bitmap [0]);
  offset = 0;
ready:
  retval = offset + page->vaddr;
  *handle = offset + page->dma;
done:
  spin_unlock_irqrestore(&pool->lock, flags);
  return retval;
}


/** */
static struct pci_page *
pool_find_page(struct pci_pool *pool, dma_addr_t dma)
{
  unsigned long flags;
  struct list_head *entry;
  struct pci_page *page;

  spin_lock_irqsave(&pool->lock, flags);
  list_for_each(entry, &pool->page_list)
    {
      page = list_entry(entry, struct pci_page, page_list);
      if (dma < page->dma)
        continue;
      if (dma < (page->dma + pool->allocation))
        goto done;
    }
  page = 0;
done:
  spin_unlock_irqrestore(&pool->lock, flags);
  return page;
}


/** Put block back into pci pool
 * \param pool   the pci pool holding the block
 * \param vaddr  virtual address of block
 * \param dma    dma address of block
 *
 * Caller promises neither device nor driver will again touch this block
 * unless it is first re-allocated.
 */
void
pci_pool_free (struct pci_pool *pool, void *vaddr, dma_addr_t dma)
{
  struct pci_page *page;
  unsigned long flags;
  int map, block;

  if ((page = pool_find_page (pool, dma)) == 0)
    {
      printk(KERN_ERR "pci_pool_free %s/%s, %p/%x (bad dma)\n",
             pool->dev ? pool->dev->slot_name : NULL,
             pool->name, vaddr, (int) (dma & 0xffffffff));
      return;
    }
#ifdef  CONFIG_PCIPOOL_DEBUG
  if (((dma - page->dma) + (void *)page->vaddr) != vaddr)
    {
      printk(KERN_ERR "pci_pool_free %s/%s, %p (bad vaddr)/%x\n",
             pool->dev ? pool->dev->slot_name : NULL,
             pool->name, vaddr, (int) (dma & 0xffffffff));
      return;
    }
#endif

  block = dma - page->dma;
  block /= pool->size;
  map = block / BITS_PER_LONG;
  block %= BITS_PER_LONG;

#ifdef  CONFIG_PCIPOOL_DEBUG
  if (page->bitmap [map] & (1UL << block))
    {
      printk(KERN_ERR "pci_pool_free %s/%s, dma %x already free\n",
             pool->dev ? pool->dev->slot_name : NULL,
             pool->name, dma);
      return;
    }
#endif
  if (pool->flags & SLAB_POISON)
    memset(vaddr, POOL_POISON_BYTE, pool->size);

  spin_lock_irqsave(&pool->lock, flags);
  set_bit(block, &page->bitmap [map]);
  if (waitqueue_active(&pool->waitq))
    wake_up(&pool->waitq);
  /*
   * Resist a temptation to do
   *    if (!is_page_busy(bpp, page->bitmap)) pool_free_page(pool, page);
   * it is not interrupt safe. Better have empty pages hang around.
   */
  spin_unlock_irqrestore(&pool->lock, flags);
}

/** Allocation of PCI consistent DMA Memory
 * \ingroup mod_pci
 *
 * \todo Is this really a PCI issue?!
 */
void *pci_alloc_consistent(struct pci_dev *hwdev,
                           size_t size, dma_addr_t * dma_handle)
{
  void *ret;
  int gfp = GFP_ATOMIC;

/* NO ISA now
   if (hwdev == NULL || hwdev->dma_mask != 0xffffffff)
   gfp |= GFP_DMA;
*/
  ret = (void *) __get_free_pages(gfp, get_order(size));

  if (ret != NULL)
    {
      memset(ret, 0, size);
      *dma_handle = virt_to_bus(ret);
    }
#if DEBUG_PALLOC
  DMSG("PCI requested pages\n");
#endif
  return ret;
}

/** Deallocation of PCI consistent DMA Memory
 * \ingroup mod_pci
 *
 * \todo Is this really a PCI issue?!
 */
void pci_free_consistent(struct pci_dev *hwdev, size_t size,
                         void *vaddr, dma_addr_t dma_handle)
{
  free_pages((unsigned long) vaddr, get_order(size));
#if DEBUG_PALLOC
  DMSG("PCI released pages\n");
#endif
}

/* XXX think about this */
int pci_set_dma_mask(struct pci_dev *dev, u64 mask)
{
  dev->dma_mask = mask;

  return 0;
}

/** @} */
/** \name Configuration space access
 * @{ */

/** PCI Configuration Space access - read byte
 * \ingroup mod_pci
 *
 * \param dev   PCI device
 * \param pos   configuration register
 *
 * \retval val  register contents
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int pci_read_config_byte(struct pci_dev *dev, int pos, l4_uint8_t * val)
{
  int err;
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      ERROR("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
    }

  err = l4io_pci_readb_cfg(pdev, pos, val);

#if DEBUG_PCI
  if (err)
    ERROR("reading PCI config register (%d)", err);
#endif

  return err ? -EIO : 0;
}

/** PCI Configuration Space access - read word
 * \ingroup mod_pci
 *
 * \param dev   PCI device
 * \param pos   configuration register
 *
 * \retval val  register contents
 * \return 0 on success; negative error code otherwise
 */
int pci_read_config_word(struct pci_dev *dev, int pos, l4_uint16_t * val)
{
  int err;
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      ERROR("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
    }

  err = l4io_pci_readw_cfg(pdev, pos, val);

#if DEBUG_PCI
  if (err)
    ERROR("reading PCI config register (%d)", err);
#endif

  return err ? -EIO : 0;
}

/** PCI Configuration Space access - read double word
 * \ingroup mod_pci
 *
 * \param dev   PCI device
 * \param pos   configuration register
 *
 * \retval val  register contents
 * \return 0 on success; negative error code otherwise
 */
int pci_read_config_dword(struct pci_dev *dev, int pos, l4_uint32_t * val)
{
  int err;
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      ERROR("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
    }

  err = l4io_pci_readl_cfg(pdev, pos, val);

#if DEBUG_PCI
  if (err)
    ERROR("reading PCI config register (%d)", err);
#endif

  return err ? -EIO : 0;
}

/** PCI Configuration Space access - write byte
 * \ingroup mod_pci
 *
 * \param dev  PCI device
 * \param pos  configuration register
 * \param val  new value
 *
 * \return 0 on success; negative error code otherwise
 */
int pci_write_config_byte(struct pci_dev *dev, int pos, l4_uint8_t val)
{
  int err;
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      ERROR("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
    }

  err = l4io_pci_writeb_cfg(pdev, pos, val);

#if DEBUG_PCI
  if (err)
    ERROR("writing PCI config register (%d)", err);
#endif

  return err ? -EIO : 0;
}

/** PCI Configuration Space access - write word
 * \ingroup mod_pci
 *
 * \param dev  PCI device
 * \param pos  configuration register
 * \param val  new value
 *
 * \return 0 on success; negative error code otherwise
 */
int pci_write_config_word(struct pci_dev *dev, int pos, l4_uint16_t val)
{
  int err;
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      ERROR("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
    }

  err = l4io_pci_writew_cfg(pdev, pos, val);

#if DEBUG_PCI
  if (err)
    ERROR("writing PCI config register (%d)", err);
#endif

  return err ? -EIO : 0;
}

/** PCI Configuration Space access - write double word
 * \ingroup mod_pci
 *
 * \param dev  PCI device
 * \param pos  configuration register
 * \param val  new value
 *
 * \return 0 on success; negative error code otherwise
 */
int pci_write_config_dword(struct pci_dev *dev, int pos, l4_uint32_t val)
{
  int err;
  l4io_pdev_t pdev;

  if (!(pdev=__pci_get_handle(dev)))
    {
#if DEBUG_PCI
      ERROR("device %p not found", dev);
#endif
      return PCIBIOS_DEVICE_NOT_FOUND;
    }

  err = l4io_pci_writel_cfg(pdev, pos, val);

#if DEBUG_PCI
  if (err)
    ERROR("writing PCI config register (%d)", err);
#endif

  return err ? -EIO : 0;
}

/** @} */
/** \name Functions for Linux backward compatibility
 * This is from drivers/pci/compat.c
 * @{ */

#if 0
/** Find ... (old interface)
 * \ingroup mod_pci */
int pcibios_find_class(unsigned int class, unsigned short index,
                       unsigned char *bus, unsigned char *devfn)
{
  const struct pci_dev *dev = NULL;
  int cnt = 0;

  while ((dev = pci_find_class(class, dev)))
    if (index == cnt++)
      {
        *bus = dev->bus->number;
        *devfn = dev->devfn;
        return PCIBIOS_SUCCESSFUL;
      }

  return PCIBIOS_DEVICE_NOT_FOUND;
}
#endif

/** Find ... (old interface)
 * \ingroup mod_pci */
int pcibios_find_device(unsigned short vendor, unsigned short device,
                        unsigned short index, unsigned char *bus,
                        unsigned char *devfn)
{
  const struct pci_dev *dev = NULL;
  int cnt = 0;

  while ((dev = pci_find_device(vendor, device, dev)))
    if (index == cnt++)
      {
        *bus = dev->bus->number;
        *devfn = dev->devfn;
        return PCIBIOS_SUCCESSFUL;
      }

  return PCIBIOS_DEVICE_NOT_FOUND;
}

/** Configuration space access function creation (old interface)
 * \ingroup mod_pci */
#define PCI_OP(rw,size,type)                                                \
int pcibios_##rw##_config_##size (unsigned char bus, unsigned char dev_fn,  \
                                  unsigned char where, unsigned type val)   \
{                                                                           \
  struct pci_dev *dev = pci_find_slot(bus, dev_fn);                         \
  if (!dev) return PCIBIOS_DEVICE_NOT_FOUND;                                \
  return pci_##rw##_config_##size(dev, where, val);                         \
}

PCI_OP(read, byte, char *)
PCI_OP(read, word, short *)
PCI_OP(read, dword, int *)
PCI_OP(write, byte, char)
PCI_OP(write, word, short)
PCI_OP(write, dword, int)

/** @} */
/** Initalize PCI module
 * \ingroup mod_pci
 *
 * \return 0 on success; negative error code otherwise
 *
 * Scan all PCI devices and establish virtual PCI bus.
 *
 * \todo consider pcibus no as parameter
 */
int l4dde_pci_init()
{
  struct pci_dev *dev = NULL;
  int err, i;
  l4io_pdev_t start = 0;
  l4io_pci_dev_t new;

  if (_initialized)
    return -L4_ESKIPPED;

  /* setup virtual bus */
  INIT_LIST_HEAD(&pcibus.devices);

  /* setup devices */
  for (;;)
    {
      if (dev && !(start=__pci_get_handle((struct pci_dev*)dev)))
        {
#if DEBUG_PCI
          PANIC("device %p not found -- Maybe you have to setup PCI_DEVICES"
                "properly (default is 12 devices maximum).", dev);
#endif
          return -L4_EUNKNOWN;
        }

      err = l4io_pci_find_device(PCI_ANY_ID, PCI_ANY_ID, start, &new);

      if (err)
        {
          if (err == -L4_ENOTFOUND) break;
#if DEBUG_PCI
          ERROR("locate PCI device (%d)", err);
#endif
          return err;
        }

      /* look for free slot */
      for (i=0; i < PCI_DEVICES; i++)
        if (!pcidevs[i].l4)
          break;
      if (i == PCI_DEVICES)
        {
#if DEBUG_PCI
          PANIC("all PCI device slots occupied");
#endif
          return -L4_EUNKNOWN;
        }

      /* save information */
      __pci_io_to_linux(&new, &pcidevs[i].linus);
      pcidevs[i].l4 = new.handle;

      dev = &pcidevs[i].linus;
    }

  ++_initialized;
  return 0;
}
