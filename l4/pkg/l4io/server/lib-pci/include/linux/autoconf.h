#include_next <linux/autoconf.h>

/* cleanup configuration */

#undef CONFIG_PCI
#undef CONFIG_PCI_BIOS
#undef CONFIG_PCI_DIRECT
#undef CONFIG_PCI_NAMES

/* configure PCI support */

#undef  CONFIG_SMP              /* not necessary for this lib */
#undef  CONFIG_MODULES          /* just to be sure here */

#undef  CONFIG_X86_IO_APIC      /* keep an eye on this! */
#undef  CONFIG_X86_LOCAL_APIC   /* keep an eye on this! */

#define CONFIG_PCI 1
#undef  CONFIG_PCI_GOBIOS       /* in config.in only */
#undef  CONFIG_PCI_GODIRECT     /* in config.in only */
#undef  CONFIG_PCI_GOANY        /* in config.in only */
#undef  CONFIG_PCI_BIOS         /* arch-i386/pci-irq.c arch-i386/pci-pc.c */
#define CONFIG_PCI_DIRECT 1     /* arch-i386/pci-pc.c */
#define CONFIG_PCI_NAMES 1      /* pci/names.c */

#undef  CONFIG_ACPI_PCI         /* arch-i386/pci-pc.c */
#undef  CONFIG_HOTPLUG          /* pci/pci.c */
#undef  CONFIG_PM               /* pci/pci.c */
#undef  CONFIG_PROC_FS          /* pci/pci.c */
#undef  CONFIG_GART_IOMMU       /* arch-i386/pci-pc.c */
#undef  CONFIG_MULTIQUAD        /* arch-i386/pci-pc.c */
