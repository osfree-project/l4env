#include <l4/util/util.h>
#include "libpci-compat24.h"

unsigned long __weak pci_mem_start = 0x40000000;

static int libpci_compat_pcidev = 0;
static int libpci_compat_pcibus = 0;
static int libpci_compat_pcires = 0;

static struct pci_dev libpci_compat_pcidevs[libpci_compat_MAX_DEV];
static struct pci_bus libpci_compat_pcibuss[libpci_compat_MAX_BUS];
static struct pci_resource libpci_compat_pciress[libpci_compat_MAX_RES];
static void*refdev[libpci_compat_MAX_DEV];
static void*refbus[libpci_compat_MAX_BUS];
static void*refres[libpci_compat_MAX_RES];

static void*alloc_entry(void**refbase, void*fieldbase,
			unsigned size, unsigned count){
    int i;

    for(i=0;i<count;i++){
	if(refbase[i]==0){
	    refbase[i]=fieldbase + i*size;
	    return refbase[i];
	}
    }
    return 0;
}

static void free_entry(void*addr, void**refbase, void*fieldbase,
		       unsigned size){
    int i;
    i = (addr-fieldbase)/size;
    refbase[i] = 0;
}

void * 
libpci_compat_kmalloc(int size)
{
    if ((size == sizeof(struct pci_dev)))
      return alloc_entry(refdev, libpci_compat_pcidevs,
			 sizeof(struct pci_dev), libpci_compat_MAX_DEV);

    if ((size == sizeof(struct pci_bus)))
      return alloc_entry(refbus, libpci_compat_pcibuss,
			 sizeof(struct pci_bus), libpci_compat_MAX_BUS);

    if ((size == sizeof(struct pci_resource)))
      return alloc_entry(refres, libpci_compat_pciress,
			 sizeof(struct pci_resource), libpci_compat_MAX_RES);

    printf(__FILE__":%s:%d failed for size %d\n",
	   __FUNCTION__, __LINE__, size);
    printf("device %d, bus %d, res %d\n",
	   libpci_compat_pcidev,libpci_compat_pcibus,
	   libpci_compat_pcires);

    return 0;
};

void libpci_compat_kfree(void*addr){
    if(addr>=(void*)libpci_compat_pcidevs &&
       addr<(void*)(&libpci_compat_pcidevs[libpci_compat_MAX_DEV])){
	free_entry(addr, refdev, libpci_compat_pcidevs,
		   sizeof(struct pci_dev));
	return;
    }
    if(addr>=(void*)libpci_compat_pcibuss &&
       addr<(void*)(&libpci_compat_pcibuss[libpci_compat_MAX_BUS])){
	free_entry(addr, refbus, libpci_compat_pcibuss,
		   sizeof(struct pci_bus));
	return;
    }
    if(addr>=(void*)libpci_compat_pciress &&
       addr<(void*)(&libpci_compat_pciress[libpci_compat_MAX_RES])){
	free_entry(addr, refres, libpci_compat_pciress,
		   sizeof(struct pci_resource));
	return;
    }
    printf(__FILE__":%s:%d failed for addr %p\n",
	   __FUNCTION__, __LINE__, addr);
}

void libpci_udelay(int usecs){
    l4_sleep(1+usecs/1000);
}


