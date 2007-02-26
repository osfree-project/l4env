/*!
 * \file   pci/lib/src24/pci-irq.c
 * \brief  irq emulation stuff for the pci lib
 *
 * \date   04/16/2002
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
#include "libpci-compat24.h"
#include <l4/pci24/asm_pci.h>

#include "pci-i386.h"

//#define __weak
#define io_apic_assign_pci_irqs	0

unsigned int __weak pcibios_irq_mask = 0xfff8;

/* we dont do any fixups here */
void __weak pcibios_fixup_irqs(void){
}

/* We dont do anything here */
static int pcibios_lookup_irq(struct pci_dev *dev, int assign){
    return 0;
}

/* we dont work with the ioapic, so nothing to do here */
void __weak pcibios_irq_init(void){
}


void __weak pcibios_enable_irq(struct pci_dev *dev)
{
        u8 pin;
        pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &pin);
        if (pin && !pcibios_lookup_irq(dev, 1) && !dev->irq) {
                char *msg;
                if (io_apic_assign_pci_irqs)
                        msg = " Probably buggy MP table.";
                else if (pci_probe & PCI_BIOS_IRQ_SCAN)
                        msg = "";
                else
                        msg = " Please try using pci=biosirq.";
                printk(KERN_WARNING "PCI: No IRQ known for interrupt pin %c of d
evice %s.%s\n",
                       'A' + pin - 1, dev->slot_name, msg);
        }
}
