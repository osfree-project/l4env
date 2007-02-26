#include "libpci-compat.h"

static int libpci_compat_pcidev = 0;
static int libpci_compat_pcibus = 0;

static struct pci_dev libpci_compat_pcidevs[libpci_compat_MAX_DEV];
static struct pci_bus libpci_compat_pcibuss[libpci_compat_MAX_BUS];

void * 
libpci_compat_kmalloc(int size)
{
    if ((size == sizeof(struct pci_dev)) &&
	(libpci_compat_pcidev < libpci_compat_MAX_DEV))
      return &libpci_compat_pcidevs[libpci_compat_pcidev++];

    if ((size == sizeof(struct pci_bus)) &&
	(libpci_compat_pcibus < libpci_compat_MAX_BUS))
      return &libpci_compat_pcibuss[libpci_compat_pcibus++];

    printf("%s failed\n", __FUNCTION__);
    printf("device %d, bus %d\n",libpci_compat_pcidev,libpci_compat_pcibus);

    return 0;
};

