/*!
 * \file   pci/lib/src24/lock.c
 * \brief  Locking implementation for single-threaded case
 *
 * \date   04/16/2002
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
#include "libpci-compat24.h"
#include <l4/pci24/pci.h>


/* Used by several spinlocks with interrupts disabling,
   aswell as by reader-writer-spinlocks */
void libpci_lock(void){}
void libpci_unlock(void){}

/* Used in bios32 access to clear interrupts on local cpu */
void libpci_lock_cli(unsigned long*x){}
void libpci_unlock_cli(unsigned long x){}

/* Linux says in ./kernel/kmod.c:
 * This is for the serialisation of device probe() functions
 * against device open() functions
 */
void libpci_lock_dev_probe(void){}
void libpci_unlock_dev_probe(void){}
