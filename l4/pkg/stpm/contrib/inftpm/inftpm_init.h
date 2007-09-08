#ifndef __INFTPM_INIT_H
#define __INFTPM_INIT_H

#include <linux/pci.h>

int __init
tpm_pci_init(struct pci_dev *pci_dev, u16 base_addr);

int __init 
tpm_dev_init(u16 config_addr, u16 base_addr);

int __exit
tpm_pci_exit(struct pci_dev *pci_dev, u16 base_addr);

int
tpm_dev_reinit(u16 config_addr, u16 base_addr);

#endif
