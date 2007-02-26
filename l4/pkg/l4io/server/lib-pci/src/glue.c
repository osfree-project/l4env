/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4io/server/lib-pci/src/glue.c
 * \brief  L4Env l4io PCIlib Linux PCI Extracts Glue Code
 *
 * \date   05/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* Linux */
#include <linux/config.h>
#include <linux/proc_fs.h>
#include <linux/ioport.h>
#include <linux/pci.h>
#include <linux/mm.h>
#include <asm/io.h>

/* L4 */
#include <l4/util/macros.h>
#include <l4/util/bitops.h>

/* OSKit */
#ifdef USE_OSKIT
#include <stdlib.h>
#include <malloc.h>
#else
/* Do not pull in standard libc headers as they collide with the Linux
 * headers */
void *malloc(size_t size);
void free(void *ptr);
unsigned int strtol(const char *nptr, char **endptr, int base);
#endif

/* local */
#include "pcilib.h"

/* I/O local */
#include "io.h"
#include "res.h"

/* show debug output */
#define DO_DEBUG 0

#ifndef NO_DOX
/* fake prototypes */
void initcall_pci_proc_init(void);
#endif

/* Align PCI memory resources to superpage boundaries (BROKEN) */
#undef CONFIG_ALIGN_RESOURCES_TO_SUPERPAGE


#undef FASTCALL
#define FASTCALL(x) __attribute__((regparm(3))) x 

/** \name Resource Management Glue for PCIlib
 *
 * Linux' PCI subsystem _expects_ the new, hierarchical resource management
 * system. So it has to be mapped to io's flat allocation scheme:
 *
 *  -# callback requests/releases only for IORESOURCE_BUSY regions
 *  -# announce top-level I/O memory regions (I/O has to initiate mappings
 *     from sigma0 or whomsoever)
 *
 * Shame: Almost all of this is from Linux 2.4.x.
 *
 * \krishna central root resources are essential - io{port,mem}_resource
 *
 * \krishna no resources can be released (this only means no HOTPLUGGING is
 * supported)
 *
 * @{ */
struct resource ioport_resource = {
  "PCI IO",
  0x0000, IO_SPACE_LIMIT,
  IORESOURCE_IO
};  /**< fake port resource (PCIlib glue) */

struct resource iomem_resource = {
  "PCI mem",
  0x00000000, 0xffffffff,
  IORESOURCE_MEM
};  /**< fake memory resource (PCIlib glue) */

/** map MMIO regions above 2GB */
unsigned long pci_mem_start = 0x80000000;

/*
 * This generates reports for /proc/ioports and /proc/iomem
 */
static char * do_resource_list(struct resource *entry, const char *fmt, int offset, char *buf, char *end)
{
  if (offset < 0)
    offset = 0;

  while (entry)
    {
      const char *name = entry->name;
      unsigned long from, to;

      if ((int) (end-buf) < 80)
        return buf;

      from = entry->start;
      to = entry->end;
      if (!name)
        name = "<BAD>";

      buf += sprintf(buf, fmt + offset, from, to, name);
      if (entry->child)
        buf = do_resource_list(entry->child, fmt, offset-2, buf, end);
      entry = entry->sibling;
    }

  return buf;
}

/** Generic resource requests.
 * \ingroup grp_pci
 *
 * \krishna it seems as Linux only sorts resources here;
 *          exception: PCI conf #1 I/O ports
 *
 * \krishna now: top-level announcement and BUSY requests
 *
 * \todo Who is in trouble if region announcement fails (no mapping into io) -
 * we or the driver servers?
 */
static struct resource *__request_resource(struct resource *root,
                                           struct resource *new)
{
  unsigned long start = new->start;
  unsigned long end = new->end;
  struct resource *tmp, **p;

  if (end < start)
    return root;
  if (start < root->start)
    return root;
  if (end > root->end)
    return root;
  p = &root->child;
  for (;;)
    {
      tmp = *p;
      if (!tmp || tmp->start > end)
        {
          /* on IORESOURCE_BUSY callback io */
          if (new->flags & IORESOURCE_BUSY)
            {
              struct resource *nr = root;

              /* find new's root's root?! */
              while (nr->parent)
                nr = nr->parent;

              /* request; return root on fail */
              if (nr == &ioport_resource)
                {
                  if (callback_request_region(start, end - start + 1))
                    return root;
                }
              else if (nr == &iomem_resource)
                {
                  if (callback_request_mem_region(start, end - start + 1))
                    return root;
                }
            }

          new->sibling = tmp;
          *p = new;
          new->parent = root;
          return NULL;
        }
      p = &tmp->sibling;
      if (tmp->end < start)
        continue;
      return tmp;
    }
}

/** Generic region requests.
 * \ingroup grp_pci
 *
 * \krishna As Linux grabs a write_lock(resource_lock), we assume
 * single-threading.
 */
struct resource *__request_region(struct resource *parent,
                                  unsigned long start, unsigned long n,
                                  const char *name)
{
  struct resource *res = kmalloc(sizeof(*res), GFP_KERNEL);

  if (res)
    {
      memset(res, 0, sizeof(*res));
      res->name = name;
      res->start = start;
      res->end = start + n - 1;
      res->flags = IORESOURCE_BUSY;

      for (;;)
        {
          struct resource *conflict;

          conflict = __request_resource(parent, res);
          if (!conflict)
            break;
          if (conflict != parent)
            {
              parent = conflict;
              if (!(conflict->flags & IORESOURCE_BUSY))
                continue;
            }

          /* Uhhuh, that didn't work out.. */
          kfree(res);
          res = NULL;
          break;
        }
    }
  return res;
}

void __release_region(struct resource *parent,
                      unsigned long start, unsigned long n)
{}

/** Generic Find empty slot in the resource tree given range and alignment.
 * \ingroup grp_pci
 */
static int __find_resource(struct resource *root, struct resource *new,
                           unsigned long size,
                           unsigned long min, unsigned long max,
                           unsigned long align,
                           void (*alignf) (void *, struct resource *,
                                           unsigned long, unsigned long),
                           void *alignf_data)
{
  struct resource *this = root->child;

  new->start = root->start;
  for (;;)
    {
      if (this)
        new->end = this->start;
      else
        new->end = root->end;
      if (new->start < min)
        new->start = min;
      if (new->end > max)
        new->end = max;
      new->start = (new->start + align - 1) & ~(align - 1);
      if (alignf)
        {
          alignf(alignf_data, new, size, align);
        }
      if (new->start < new->end && new->end - new->start + 1 >= size)
        {
          new->end = new->start + size - 1;
          return 0;
        }
      if (!this)
        break;
      new->start = this->end + 1;
      this = this->sibling;
    }
  return -EBUSY;
}

/** Linux resource requests.
 * \ingroup grp_pci
 *
 * \krishna As Linux grabs a write_lock(resource_lock), we assume single
 * threading.
 *
 * \krishna Try to align MMIO regions before doing real allocation (see also
 * allocate_resource()).
 */
int request_resource(struct resource *root, struct resource *new)
{
  struct resource *conflict;

#ifdef CONFIG_ALIGN_RESOURCES_TO_SUPERPAGE
  int err;
  unsigned long size = new->end - new->start + 1;
  unsigned long min = pci_mem_start, max = -1;  /* XXX don't know if this always works */
  unsigned long align;

  LOGd(DO_DEBUG, "REQ: <0x%08lx-0x%08lx>", new->start, new->end);

  /* align MMIO regions only */
  if (new->flags & IORESOURCE_MEM)
    {
      align = l4util_bsr(size);
      /* round up */
      if (size > (1UL << align))
        align++;

      if (align < L4_LOG2_SUPERPAGESIZE)
        align = L4_LOG2_SUPERPAGESIZE;
      align = 1UL << align;

      LOGd(DO_DEBUG, "ALIGN: for 0x%08lx = 0x%08lx", size, align);

      err = __find_resource(root, new, size, min, max, align, 0, 0);
      if (err)
        return err;


      LOGd(DO_DEBUG, "ALIGN: <0x%08lx-0x%08lx>", new->start, new->end);

      /* update of PCI configuration space deferred */
    }
#endif
  conflict = __request_resource(root, new);

  return conflict ? -EBUSY : 0;
}

/** Linux resource allocation
 * \ingroup grp_pci
 *
 * \krishna As Linux grabs a write_lock(resource_lock), we assume
 * single-threading.
 */
int allocate_resource(struct resource *root, struct resource *new,
                      unsigned long size,
                      unsigned long min, unsigned long max,
                      unsigned long align,
                      void (*alignf) (void *, struct resource *,
                                      unsigned long, unsigned long),
                      void *alignf_data)
{
  int err;

  err = __find_resource(root, new, size, min, max, align, alignf, alignf_data);
  if (err >= 0 && __request_resource(root, new))
    err = -EBUSY;

  return err;
}

/** @} */
/** \name Interrupt Line Request Glue for PCIlib
 *
 * Mapping of PCIlib interrupt allocation.
 *
 * @{ */

/** Request interrupt.
 * \ingroup grp_pci
 *
 * It is used to check IRQ availability during pci_enable_device and will
 * always succeed because is executed on L4IO startup.
 */
int request_irq(unsigned int irq, void (*handler) (int, void *, struct pt_regs *),
                unsigned long flags, const char *name, void *id)
{
  /* always succeeds */
  return 0;
}

/** Free Interrupt.
 * \ingroup grp_pci
 *
 * It is used to check IRQ availability during pci_enable_device and as we
 * don't really occupy IRQs nothing is done in here.
 */
void free_irq(unsigned int irq, void *id)
{
}

/** @} */
/** \name Miscellaneous Glue for PCIlib
 *
 * - dev_probe_lock
 * - dev_probe_unlock
 * - simple_strtol (mapped to strtol)
 * - schedule_timeout
 * - ...
 *
 * @{ */

/** dev_probe_lock */
void dev_probe_lock(void)
{
}

/** dev_probe_unlock */
void dev_probe_unlock(void)
{
}

/** simple_strtol
 *
 * \krishna base is `unsigned int' NOT `int', but it's always `0' in the code
 * (as far as I know)
 */
long simple_strtol(const char *cp, char **endp, unsigned int base)
{
  return strtol(cp, endp, base);
}

/** schedule_timeout */
signed long FASTCALL(schedule_timeout(signed long timeout))
{
  /* FIXME: delay some time */
  return 0;
}

void __const_udelay(unsigned long usecs)
{
  /* FIXME: delay some time */
}

/* These are used by dead code only. */
#include <asm/processor.h>
struct cpuinfo_x86 boot_cpu_data;

int remap_page_range(unsigned long from, unsigned long to, unsigned long size,
                     pgprot_t prot)
{return 0;}
void *pci_alloc_consistent(struct pci_dev *hwdev, size_t size,
                           dma_addr_t *dma_handle)
{return (void*)0;}
void pci_free_consistent(struct pci_dev *hwdev, size_t size,
                         void *vaddr, dma_addr_t dma_handle)
{}
void FASTCALL(add_wait_queue(wait_queue_head_t *q, wait_queue_t * wait))
{}
void FASTCALL(remove_wait_queue(wait_queue_head_t *q, wait_queue_t * wait))
{}
void FASTCALL(__wake_up(wait_queue_head_t *q, unsigned int mode, int nr))
{}
/** @} */
/**
 * \name Memory Management Glue for PCIlib
 *
 * @{ */

/** Well known kmalloc.
 * \ingroup grp_pci
 *
 * \krishna Okay, PCIlib never kmallocs with gfp != GFP_KERNEL, so malloc seems
 * sufficient.
 */
void *kmalloc(size_t size, int gfp)
{
  /* malloc size */
  return malloc(size);
}

/** Well known kfree.
 * \ingroup grp_pci
 */
void kfree(const void *obj)
{
  /* free obj */
  free((void *) obj);
}

/** Well known __get_free_pages.
 * \ingroup grp_pci
 *
 * \krishna It's called by \c pci_alloc_consistent() and \c
 * pcibios_get_irq_routing_table() - so do we _really_ need it?
 *
 * \todo implement __get_free_pages using l4rm?! ... for now it's _not_
 */
unsigned long FASTCALL(__get_free_pages(unsigned int gfp, unsigned int order))
{
  unsigned long something = 0;

  return something;
}

/** Well known free_pages.
 * \ingroup grp_pci
 *
 * \krishna It's called by \c pci_free_consistent() and \c
 * pcibios_get_irq_routing_table() - so do we _really_ need it?
 *
 * \todo implement free_pages using l4rm?! ... for now it's _not_
 */
void FASTCALL(free_pages(unsigned long addr, unsigned int order))
{
  LOGdL(DO_DEBUG, "(%#08lx, %i)", addr, order);
}

/** @} */
/** \name PCIlib interface
 *
 * \todo /proc fs has to be optional
 *
 * @{ */

/** PCIlib initialization.
 * \ingroup grp_pci */
int PCI_init(int list)
{
#ifdef CONFIG_ALIGN_RESOURCES_TO_SUPERPAGE
  struct list_head *list;
#endif
  struct pci_dev *p;

  pci_init();

  pci_for_each_dev(p)
    {
      int i;

      /* enable/disable device to update resource allocations (interrupts, ...)
         This fixes a bug detected using USB hubs that allocate their "real"
         IRQs _after_ pci_enable_device(). thanks to gg5@os */
      if ((i = pci_enable_device(p)))
        LOG("WARNING: initial PCI device activation for %s failed (%d)\n",
            p->slot_name, i);
      /* Removed pci_disable_device(p); here because it produced errors with
         non-compliant drivers from grub. */

      for (i = 0; i < 6; i++)
        {
          if (p->resource[i].flags & IORESOURCE_MEM)
            {
#ifdef CONFIG_ALIGN_RESOURCES_TO_SUPERPAGE
              /* update configuration space of all PCI devices */
              pcibios_update_resource(p, &iomem_resource, &p->resource[i], i);
#endif

              /* announce memory region */
              if (callback_handle_pci_device(p->vendor, p->device))
                callback_announce_mem_region(p->resource[i].start,
                                             p->resource[i].end-p->resource[i].start+1);
              else
                printf("Ignoring memory %08lx-%08lx for device %04x:%04x\n",
                       p->resource[i].start,
                       p->resource[i].end,
                       p->vendor, p->device);
            }
        }
    }

  if (list)
  {
    char *buf = malloc(4000);
    if (buf)
      {
        do_resource_list(&iomem_resource, "        %08lx-%08lx : %s\n", 8, buf, &buf[3999]);
        printf("%s", buf);
        do_resource_list(&ioport_resource, "        %04lx-%04lx : %s\n", 8, buf, &buf[3999]);
        printf("%s", buf);
        free(buf);
      }
  }

  return 0;
}

/** Info Type Conversion.
 * \ingroup grp_pci
 *
 * \return concatenated bus number and devfn index
 */
unsigned short PCI_linux_to_io(void *linux_pdev, void *l4io_pdev)
{
  int i;
  l4io_pci_dev_t *l4io = (l4io_pci_dev_t *) l4io_pdev;
  struct pci_dev *linus = (struct pci_dev *) linux_pdev;

  l4io->bus = (unsigned char)(linus->bus->number & 0xff);
  l4io->devfn = (unsigned char)(linus->devfn & 0xff);
  l4io->vendor = linus->vendor;
  l4io->device = linus->device;
  l4io->sub_vendor = linus->subsystem_vendor;
  l4io->sub_device = linus->subsystem_device;
  l4io->dev_class = linus->class;

  l4io->irq = linus->irq;
  for (i = 0; i < 12; i++)
    {
      l4io->res[i].start = linus->resource[i].start;
      l4io->res[i].end = linus->resource[i].end;
      l4io->res[i].flags = linus->resource[i].flags;
    }

  strcpy(&l4io->name[0], &linus->name[0]);
  strcpy(&l4io->slot_name[0], &linus->slot_name[0]);

  return (l4io->bus<<8 | l4io->devfn);
}
/** @} */
