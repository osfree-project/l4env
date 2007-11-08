#include_next <linux/autoconf.h>

#undef CONFIG_USB
#undef CONFIG_USB_EHCI_HCD
#undef CONFIG_USB_UHCI
#undef CONFIG_USB_OHCI
#undef CONFIG_USB_HID
#undef CONFIG_USB_HIDINPUT

/*
 * USB support
 */
#define CONFIG_USB 1
#undef  CONFIG_USB_DEBUG

/*
 * Miscellaneous USB options
 */
#undef  CONFIG_USB_DEVICEFS
#undef  CONFIG_USB_BANDWIDTH

/*
 * USB Host Controller Drivers
 */
#define CONFIG_USB_EHCI_HCD 1
#define CONFIG_USB_UHCI 1
#undef  CONFIG_USB_UHCI_ALT
#define CONFIG_USB_OHCI 1
#undef  CONFIG_USB_SL811HS_ALT
#undef  CONFIG_USB_SL811HS

/*
 * USB Device Class drivers
 */
#undef  CONFIG_USB_AUDIO
#undef  CONFIG_USB_EMI26
#undef  CONFIG_USB_MIDI
#undef  CONFIG_USB_STORAGE
#undef  CONFIG_USB_STORAGE_DEBUG
#undef  CONFIG_USB_STORAGE_DATAFAB
#undef  CONFIG_USB_STORAGE_FREECOM
#undef  CONFIG_USB_STORAGE_ISD200
#undef  CONFIG_USB_STORAGE_DPCM
#undef  CONFIG_USB_STORAGE_HP8200e
#undef  CONFIG_USB_STORAGE_SDDR09
#undef  CONFIG_USB_STORAGE_SDDR55
#undef  CONFIG_USB_STORAGE_JUMPSHOT
#undef  CONFIG_USB_ACM
#undef  CONFIG_USB_PRINTER

/*
 * USB Human Interface Devices (HID)
 */
#define CONFIG_USB_HID 1

/*
 *     Input core support is needed for USB HID input layer or HIDBP support
 */
#define CONFIG_USB_HIDINPUT 1
#undef  CONFIG_USB_HIDDEV
#undef  CONFIG_USB_KBD
#undef  CONFIG_USB_MOUSE
#undef  CONFIG_USB_AIPTEK
#undef  CONFIG_USB_WACOM
#undef  CONFIG_USB_KBTAB
#undef  CONFIG_USB_POWERMATE

/*
 * USB Imaging devices
 */
#undef  CONFIG_USB_DC2XX
#undef  CONFIG_USB_MDC800
#undef  CONFIG_USB_SCANNER
#undef  CONFIG_USB_MICROTEK
#undef  CONFIG_USB_HPUSBSCSI

/*
 * USB Multimedia devices
 */

/*
 *   Video4Linux support is needed for USB Multimedia device support
 */

/*
 * USB Network adaptors
 */
#undef  CONFIG_USB_PEGASUS
#undef  CONFIG_USB_RTL8150
#undef  CONFIG_USB_KAWETH
#undef  CONFIG_USB_CATC
#undef  CONFIG_USB_CDCETHER
#undef  CONFIG_USB_USBNET

/*
 * USB port drivers
 */
#undef  CONFIG_USB_USS720

/*
 * USB Serial Converter support
 */
#undef  CONFIG_USB_SERIAL

/*
 * USB Miscellaneous drivers
 */
#undef  CONFIG_USB_RIO500
#undef  CONFIG_USB_AUERSWALD
#undef  CONFIG_USB_TIGL
#undef  CONFIG_USB_BRLVGER
#undef  CONFIG_USB_LCD

/*
 * Support for USB gadgets
 */
#undef  CONFIG_USB_GADGET

