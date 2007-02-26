#ifndef __PCI_H__
#define __PCI_H__

struct pci_device
  {
    unsigned short vendor, dev_id;
    const char *name;
    unsigned int membase;
    unsigned short ioaddr;
    unsigned short devfn;
    unsigned short bus;
  };

#endif /* __PCI_H__ */
