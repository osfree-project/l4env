#include_next <linux/autoconf.h>

#undef CONFIG_USB_EHCI_SPLIT_ISO
#define CONFIG_USB_EHCI_SPLIT_ISO 1

#undef CONFIG_PM
#undef CONFIG_USB_DEVICEFS
#undef CONFIG_USB_SUSPEND
