/*!
 * \file   pci/examples/main.c
 * \brief  PCI example: List all devices
 *
 * \date   04/16/2002
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */

#include <stdio.h>
#include <l4/log/l4log.h>
#include <l4/pci24/pci.h>
#include <l4/sys/kdebug.h>

char LOG_tag[9]="pcidevs";

int main(int argc, char**argv){
    struct pci_dev *dev=0;
    const char*class;
    pci_init();

    printf("Here we are, listing the pci-devices:\n");
    for(dev = pci_find_device(PCI_ANY_ID, PCI_ANY_ID, 0);
	dev;
	dev = pci_find_device(PCI_ANY_ID, PCI_ANY_ID, dev)){
	class = pci_class_name(dev->class>>8);
	printf("Bus %2x, device %3x, function %2x - %s\n",
	       dev->bus->number, PCI_SLOT(dev->devfn), PCI_FUNC(dev->devfn),
	       class?class:"unknown class");
	printf("  %s, IRQ %X\n", dev->name, dev->irq);
    }
    printf("Thats all\n");
    LOG_flush();
    enter_kdebug(".");
    return 0;
}
