/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_io/include/ARCH-x86/libio.h
 * \brief  L4Env I/O Client API
 *
 * \date   05/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __GENERIC_IO_INCLUDE_ARCH_X86_LIBIO_H_
#define __GENERIC_IO_INCLUDE_ARCH_X86_LIBIO_H_

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/generic_io/types.h>

/*****************************************************************************/
/** Initialize IO library.
 * \ingroup grp_misc
 *
 * \param  io_info_addr  mapping address of io info page:
 *                       - 0: libio queries RM for appropriate region
 *                       - -1: no mapping is done at all
 *                       - otherwise \a io_info_addr is used; area has to be
 *                         prereserved at RM
 * \param  drv_type      short driver description
 *
 * \retval io_info_addr actual mapping address (or -1 if no mapping)
 *
 * \return 0 on success, negative error code otherwise
 *
 * This initializes libio:
 *
 * - register driver according to drv_type
 * - request and map io info page
 *
 * Before io info page is mapped into client's address space any potentially
 * mapping at the given address is FLUSHED! \a io_info_addr has to be pagesize
 * aligned.
 */
/*****************************************************************************/
int l4io_init(l4io_info_t **io_info_addr, l4io_drv_t drv_type);

/*****************************************************************************/
/**
 * \brief  Request I/O memory region.
 * \ingroup grp_res
 *
 * \param  start   begin of mem region
 * \param  len     size of mem region
 * \retval offset  offset with memory region
 *
 * \return virtual address of mapped region; 0 on error
 */
/*****************************************************************************/
l4_addr_t l4io_request_mem_region(l4_addr_t start, l4_size_t len,
                                  l4_umword_t *offset);

/******************************************************************************/
/**
 * \brief  Search I/O memory region for an address.
 * \ingroup grp_res
 *
 * \param  addr   Address to search for
 * \retval start  Start of memory region if found
 * \retval len    Length of memory region if found.
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int l4io_search_mem_region(l4_addr_t addr,
                           l4_addr_t *start, l4_size_t *len);

/******************************************************************************/
/**
 * \brief  Release I/O memory region.
 * \ingroup grp_res
 *
 * \param  start  begin of mem region
 * \param  len    size of mem region
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int l4io_release_mem_region(l4_addr_t start, l4_size_t len);

/*****************************************************************************/
/**
 * \brief  Request I/O port region.
 * \ingroup grp_res
 *
 * \param  start  begin of port region
 * \param  len    size of port region
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int l4io_request_region(l4_addr_t start, l4_size_t len);

/*****************************************************************************/
/**
 * \brief  Release I/O port region.
 * \ingroup grp_res
 *
 * \param  start  begin of port region
 * \param  len    size of port region
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int l4io_release_region(l4_addr_t start, l4_size_t len);

/*****************************************************************************/
/**
 * \brief  Request ISA DMA channel.
 * \ingroup grp_res
 *
 * \param  channel  ISA DMA channel number
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int l4io_request_dma(unsigned int channel);

/*****************************************************************************/
/**
 * \brief  Release ISA DMA channel.
 * \ingroup grp_res
 *
 * \param  channel  ISA DMA channel number
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int l4io_release_dma(unsigned int channel);

/******************************************************************************/
/**
 * \brief  Find PCI device on given bus and slot.
 * \ingroup grp_pci
 *
 * \param  bus   PCI bus number
 * \param  slot  PCI slot number
 *
 * \retval pci_dev
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int l4io_pci_find_slot(unsigned int bus, unsigned int slot,
                       l4io_pci_dev_t * pci_dev);

/*****************************************************************************/
/**
 * \brief  Find PCI device on given vendor and device ids.
 * \ingroup grp_pci
 *
 * \param  vendor   vendor id or ~0 for any
 * \param  device   device id or ~0 for any
 * \param  start    previous PCI device found, or 0 for new search
 *
 * \retval pci_dev
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int l4io_pci_find_device(unsigned short vendor, unsigned short device,
                         l4io_pdev_t start, l4io_pci_dev_t * pci_dev);

/*****************************************************************************/
/**
 * \brief  Find PCI device on given class id.
 * \ingroup grp_pci
 *
 * \param  class  class id
 * \param  start  previous PCI device found, or 0 for new search
 *
 * \retval pci_dev
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int l4io_pci_find_class(unsigned long class,
                        l4io_pdev_t start, l4io_pci_dev_t * pci_dev);

/*****************************************************************************/
/**
 * \brief  Initialize PCI device.
 * \ingroup grp_pci
 *
 * \param  pdev  PCI device handle
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int l4io_pci_enable(l4io_pdev_t pdev);

/*****************************************************************************/
/**
 * \brief  Finalize PCI device.
 * \ingroup grp_pci
 *
 * \param  pdev  PCI device handle
 *
 * \return 0 on success; negative error code otherwise
 */
/*****************************************************************************/
int l4io_pci_disable(l4io_pdev_t pdev);

/*****************************************************************************/
/**
 * \brief  Enable busmastering for PCI device.
 * \ingroup grp_pci
 *
 * \param  pdev  PCI device handle
 */
/*****************************************************************************/
void l4io_pci_set_master(l4io_pdev_t pdev);

/*****************************************************************************/
/**
 * \brief  Set PM state for PCI device.
 * \ingroup grp_pci
 *
 * \param  pdev   PCI device handle
 * \param  state  new PM state
 *
 * \return old PM state
 */
/*****************************************************************************/
int l4io_pci_set_pm(l4io_pdev_t pdev, int state);

/*****************************************************************************/
/**
 * \name  Read PCI configuration registers.
 * \ingroup grp_pci
 *
 * \param  pdev  PCI device handle
 * \param  pos   PCI configuration register
 *
 * \retval val   register value
 *
 * \return 0 on success; negative error code otherwise
 *
 * @{ */
/*****************************************************************************/
int l4io_pci_readb_cfg(l4io_pdev_t pdev, int pos, l4_uint8_t * val);
int l4io_pci_readw_cfg(l4io_pdev_t pdev, int pos, l4_uint16_t * val);
int l4io_pci_readl_cfg(l4io_pdev_t pdev, int pos, l4_uint32_t * val);

/** @} */
/*****************************************************************************/
/**
 * \name  Write PCI configuration registers.
 * \ingroup grp_pci
 *
 * \param  pdev  PCI device handle
 * \param  pos   PCI configuration register
 * \param  val   register value
 *
 * \return 0 on success; negative error code otherwise
 *
 * @{ */
/*****************************************************************************/
int l4io_pci_writeb_cfg(l4io_pdev_t pdev, int pos, l4_uint8_t val);
int l4io_pci_writew_cfg(l4io_pdev_t pdev, int pos, l4_uint16_t val);
int l4io_pci_writel_cfg(l4io_pdev_t pdev, int pos, l4_uint32_t val);
/** @} */

#endif
