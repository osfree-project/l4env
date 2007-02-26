#ifndef __CARDS_H__
#define __CARDS_H__

struct nic *
eepro100_probe(struct nic *, unsigned short *, struct pci_device *);

struct nic *
pcnet32_probe(struct nic *, unsigned short *, struct pci_device *);

struct nic *
tulip_probe(struct nic *, unsigned short *, struct pci_device *);

struct nic *
nepci_probe(struct nic *, unsigned short *, struct pci_device *);

struct nic *
ne_probe(struct nic *, unsigned short *, struct pci_device *);

struct nic *
t595_probe(struct nic *, unsigned short *, struct pci_device *);

struct nic *
a3c90x_probe(struct nic *, unsigned short *, struct pci_device *);

struct nic *
t509_probe(struct nic *, unsigned short *, struct pci_device *);

struct nic *
rtl8139_probe(struct nic *nic, unsigned short *, struct pci_device *);

struct nic *
oshkosh_probe(struct nic *, unsigned short *, struct pci_device *);

#endif

