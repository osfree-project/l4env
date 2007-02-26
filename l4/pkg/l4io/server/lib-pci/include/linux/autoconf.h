/* $Id$ */

/*
 * Automatically generated C config: don't edit
 */

/*
 * Modified for PCIlib ...
 */

#define AUTOCONF_INCLUDED
#define CONFIG_X86 1
#define CONFIG_ISA 1
#undef  CONFIG_SBUS
#define CONFIG_UID16 1
/*
 * Code maturity level options
 */
#define CONFIG_EXPERIMENTAL 1
/*
 * Loadable module support
 */
#undef  CONFIG_MODULES
/*
 * Processor type and features
 */
#undef  CONFIG_M386
#undef  CONFIG_M486
#undef  CONFIG_M586
#define CONFIG_M586TSC 1
#undef  CONFIG_M586MMX
#undef  CONFIG_M686
#undef  CONFIG_MPENTIUMIII
#undef  CONFIG_MPENTIUM4
#undef  CONFIG_MK6
#undef  CONFIG_MK7
#undef  CONFIG_MCRUSOE
#undef  CONFIG_MWINCHIPC6
#undef  CONFIG_MWINCHIP2
#undef  CONFIG_MWINCHIP3D
#define CONFIG_X86_WP_WORKS_OK 1
#define CONFIG_X86_INVLPG 1
#define CONFIG_X86_CMPXCHG 1
#define CONFIG_X86_BSWAP 1
#define CONFIG_X86_POPAD_OK 1
#define CONFIG_X86_L1_CACHE_SHIFT (5)
#define CONFIG_X86_USE_STRING_486 1
#define CONFIG_X86_ALIGNMENT_16 1
#define CONFIG_X86_TSC 1
#undef  CONFIG_TOSHIBA
#undef  CONFIG_MICROCODE
#undef  CONFIG_X86_MSR
#undef  CONFIG_X86_CPUID
#define CONFIG_NOHIGHMEM 1
#undef  CONFIG_HIGHMEM4G
#undef  CONFIG_HIGHMEM64G
#undef  CONFIG_MATH_EMULATION
#undef  CONFIG_MTRR
#undef  CONFIG_SMP
#undef  CONFIG_X86_UP_IOAPIC
/*
 * General setup
 */
#undef  CONFIG_NET
#undef  CONFIG_VISWS
#undef  CONFIG_X86_IO_APIC	/* keep an eye on this! */
#undef  CONFIG_X86_LOCAL_APIC	/* keep an eye on this! */
#define CONFIG_PCI 1
#undef  CONFIG_PCI_GOBIOS
#undef  CONFIG_PCI_GODIRECT
#define CONFIG_PCI_GOANY 1
#undef  CONFIG_PCI_BIOS	/* arch-i386/pci-irq.c
				 * arch-i386/pci-pc.c */
#define CONFIG_PCI_DIRECT 1	/* arch-i386/pci-pc.c */
#define CONFIG_PCI_NAMES 1	/* pci/names.c */
#undef  CONFIG_EISA
#undef  CONFIG_MCA
#undef	CONFIG_HOTPLUG		/* pci/pci.c */
#undef  CONFIG_PCMCIA
#undef  CONFIG_SYSVIPC
#undef  CONFIG_BSD_PROCESS_ACCT
#define CONFIG_SYSCTL 1
#undef  CONFIG_BINFMT_AOUT
#define CONFIG_BINFMT_ELF 1
#undef  CONFIG_BINFMT_MISC
#undef	CONFIG_PM		/* pci/pci.c */
#undef  CONFIG_APM_IGNORE_USER_SUSPEND
#undef  CONFIG_APM_DO_ENABLE
#undef  CONFIG_APM_CPU_IDLE
#undef  CONFIG_APM_DISPLAY_BLANK
#undef  CONFIG_APM_RTC_IS_GMT
#undef  CONFIG_APM_ALLOW_INTS
#undef  CONFIG_APM_REAL_MODE_POWER_OFF
/*
 * Memory Technology Devices (MTD)
 */
#undef  CONFIG_MTD
/*
 * Parallel port support
 */
#undef  CONFIG_PARPORT
/*
 * Plug and Play configuration
 */
#undef  CONFIG_PNP
/*
 * Block devices
 */
#define CONFIG_BLK_DEV_FD 1
#undef  CONFIG_BLK_DEV_XD
#undef  CONFIG_BLK_CPQ_DA
#undef  CONFIG_BLK_CPQ_CISS_DA
#undef  CONFIG_BLK_DEV_DAC960
#undef  CONFIG_BLK_DEV_LOOP
#undef  CONFIG_BLK_DEV_RAM
/*
 * Multi-device support (RAID and LVM)
 */
#undef  CONFIG_MD
/*
 * Telephony Support
 */
#undef  CONFIG_PHONE
/*
 * ATA/IDE/MFM/RLL support
 */
#undef  CONFIG_IDE
#undef  CONFIG_BLK_DEV_IDE_MODES
#undef  CONFIG_BLK_DEV_HD
/*
 * SCSI support
 */
#undef  CONFIG_SCSI
/*
 * IEEE 1394 (FireWire) support
 */
#undef  CONFIG_IEEE1394
/*
 * I2O device support
 */
#undef  CONFIG_I2O
/*
 * Amateur Radio support
 */
#undef  CONFIG_HAMRADIO
/*
 * ISDN subsystem
 */
/*
 * Old CD-ROM drivers (not SCSI, not IDE)
 */
#undef  CONFIG_CD_NO_IDESCSI
/*
 * Input core support
 */
#undef  CONFIG_INPUT
/*
 * Character devices
 */
#define CONFIG_VT 1
#define CONFIG_VT_CONSOLE 1
#undef  CONFIG_SERIAL
#undef  CONFIG_SERIAL_NONSTANDARD
#undef  CONFIG_UNIX98_PTYS
/*
 * I2C support
 */
#undef  CONFIG_I2C
/*
 * Mice
 */
#undef  CONFIG_BUSMOUSE
#undef  CONFIG_MOUSE
/*
 * Joysticks
 */
/*
 * Game port support
 */
/*
 * Gameport joysticks
 */
/*
 * Serial port support
 */
/*
 * Serial port joysticks
 */
/*
 * Parallel port joysticks
 */
/*
 *   Parport support is needed for parallel port joysticks
 */
#undef  CONFIG_QIC02_TAPE
/*
 * Watchdog Cards
 */
#undef  CONFIG_WATCHDOG
#undef  CONFIG_INTEL_RNG
#undef  CONFIG_NVRAM
#undef  CONFIG_RTC
#undef  CONFIG_DTLK
#undef  CONFIG_R3964
#undef  CONFIG_APPLICOM
/*
 * Ftape, the floppy tape device driver
 */
#undef  CONFIG_FTAPE
#undef  CONFIG_AGP
#undef  CONFIG_DRM
/*
 * Multimedia devices
 */
#undef  CONFIG_VIDEO_DEV
/*
 * File systems
 */
#undef  CONFIG_QUOTA
#undef  CONFIG_AUTOFS_FS
#undef  CONFIG_AUTOFS4_FS
#undef  CONFIG_REISERFS_FS
#undef  CONFIG_ADFS_FS
#undef  CONFIG_AFFS_FS
#undef  CONFIG_HFS_FS
#undef  CONFIG_BFS_FS
#undef  CONFIG_FAT_FS
#undef  CONFIG_EFS_FS
#define CONFIG_JFFS_FS_VERBOSE (0)
#undef  CONFIG_CRAMFS
#undef  CONFIG_RAMFS
#undef  CONFIG_ISO9660_FS
#undef  CONFIG_MINIX_FS
#undef  CONFIG_NTFS_FS
#undef  CONFIG_HPFS_FS
#define CONFIG_PROC_FS		/* pci/proc.c */
#undef  CONFIG_DEVFS_FS
#undef  CONFIG_QNX4FS_FS
#undef  CONFIG_ROMFS_FS
#undef  CONFIG_EXT2_FS
#undef  CONFIG_SYSV_FS
#undef  CONFIG_UDF_FS
#undef  CONFIG_UFS_FS
#undef  CONFIG_NCPFS_NLS
#undef  CONFIG_SMB_FS
/*
 * Partition Types
 */
#undef  CONFIG_PARTITION_ADVANCED
#define CONFIG_MSDOS_PARTITION 1
#undef  CONFIG_SMB_NLS
#undef  CONFIG_NLS
/*
 * Console drivers
 */
#define CONFIG_VGA_CONSOLE 1
#undef  CONFIG_VIDEO_SELECT
#undef  CONFIG_MDA_CONSOLE
/*
 * Frame-buffer support
 */
#undef  CONFIG_FB
/*
 * Sound
 */
#undef  CONFIG_SOUND
/*
 * USB support
 */
#undef  CONFIG_USB
/*
 * Kernel hacking
 */
#undef  CONFIG_MAGIC_SYSRQ
