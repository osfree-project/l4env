/*
 * Automatically generated C config: don't edit
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
#undef  CONFIG_KMOD
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
#undef CONFIG_X86_ALIGNMENT_16
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
#define CONFIG_SMP 1
#define CONFIG_HAVE_DEC_LOCK 1

/*
 * General setup
 */
#define CONFIG_NET 1
#undef  CONFIG_VISWS
#define CONFIG_X86_IO_APIC 1
#define CONFIG_X86_LOCAL_APIC 1
#define CONFIG_PCI 1
#undef  CONFIG_PCI_GOBIOS
#undef  CONFIG_PCI_GODIRECT
#define CONFIG_PCI_GOANY 1
#define CONFIG_PCI_BIOS 1
#define CONFIG_PCI_DIRECT 1
#define CONFIG_PCI_NAMES 1
#undef  CONFIG_EISA
#undef  CONFIG_MCA
#undef  CONFIG_HOTPLUG
#undef  CONFIG_PCMCIA
#undef  CONFIG_SYSVIPC
#undef  CONFIG_BSD_PROCESS_ACCT
#define CONFIG_SYSCTL 1
#define CONFIG_KCORE_ELF 1
#undef  CONFIG_KCORE_AOUT
#undef  CONFIG_BINFMT_AOUT
#define CONFIG_BINFMT_ELF 1
#undef  CONFIG_BINFMT_MISC
#undef  CONFIG_PM
#undef  CONFIG_ACPI
#undef  CONFIG_APM

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
#undef  CONFIG_ISAPNP

/*
 * Block devices
 */
#define CONFIG_BLK_DEV_FD 1
#undef  CONFIG_BLK_DEV_XD
#undef  CONFIG_PARIDE
#undef  CONFIG_BLK_CPQ_DA
#undef  CONFIG_BLK_CPQ_CISS_DA
#undef  CONFIG_BLK_DEV_DAC960
#undef  CONFIG_BLK_DEV_LOOP
#undef  CONFIG_BLK_DEV_NBD
#undef  CONFIG_BLK_DEV_RAM
#undef  CONFIG_BLK_DEV_INITRD

/*
 * Multi-device support (RAID and LVM)
 */
#undef  CONFIG_MD
/*
 * Networking options
 */
#define CONFIG_PACKET 1
#undef  CONFIG_PACKET_MMAP
#undef  CONFIG_NETLINK
#undef  CONFIG_NETFILTER
#undef  CONFIG_FILTER
#undef  CONFIG_UNIX
#define CONFIG_INET 1
#undef  CONFIG_IP_MULTICAST
#undef  CONFIG_IP_ADVANCED_ROUTER
#undef  CONFIG_IP_PNP
#undef  CONFIG_NET_IPIP
#undef  CONFIG_NET_IPGRE
#undef  CONFIG_INET_ECN
#undef  CONFIG_SYN_COOKIES
#undef  CONFIG_IPV6
#undef  CONFIG_KHTTPD
#undef  CONFIG_ATM
/*
 *  
 */
#undef  CONFIG_IPX
#undef  CONFIG_ATALK
#undef  CONFIG_DECNET
#undef  CONFIG_BRIDGE
#undef  CONFIG_X25
#undef  CONFIG_LAPB
#undef  CONFIG_LLC
#undef  CONFIG_NET_DIVERT
#undef  CONFIG_ECONET
#undef  CONFIG_WAN_ROUTER
#undef  CONFIG_NET_FASTROUTE
#undef  CONFIG_NET_HW_FLOWCONTROL
/*
 * QoS and/or fair queueing
 */
#undef  CONFIG_NET_SCHED
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
#define CONFIG_SCSI 1
/*
 * SCSI support type (disk, tape, CD-ROM)
 */
#define CONFIG_BLK_DEV_SD 1
#define CONFIG_SD_EXTRA_DEVS (40)
#undef  CONFIG_CHR_DEV_ST
#undef  CONFIG_CHR_DEV_OSST
#undef  CONFIG_BLK_DEV_SR
#undef  CONFIG_CHR_DEV_SG
/*
 * Some SCSI devices (e.g. CD jukebox) support multiple LUNs
 */
#undef  CONFIG_SCSI_DEBUG_QUEUES
#undef  CONFIG_SCSI_MULTI_LUN
#undef  CONFIG_SCSI_CONSTANTS
#undef  CONFIG_SCSI_LOGGING
/*
 * SCSI low-level drivers
 */
#undef  CONFIG_BLK_DEV_3W_XXXX_RAID
#undef  CONFIG_SCSI_7000FASST
#undef  CONFIG_SCSI_ACARD
#undef  CONFIG_SCSI_AHA152X
#undef  CONFIG_SCSI_AHA1542
#undef  CONFIG_SCSI_AHA1740
#undef  CONFIG_SCSI_AIC7XXX
#undef  CONFIG_SCSI_ADVANSYS
#undef  CONFIG_SCSI_IN2000
#undef  CONFIG_SCSI_AM53C974
#undef  CONFIG_SCSI_MEGARAID
#define CONFIG_SCSI_BUSLOGIC 1
#undef  CONFIG_SCSI_OMIT_FLASHPOINT
#undef  CONFIG_SCSI_CPQFCTS
#undef  CONFIG_SCSI_DMX3191D
#undef  CONFIG_SCSI_DTC3280
#undef  CONFIG_SCSI_EATA
#undef  CONFIG_SCSI_EATA_DMA
#undef  CONFIG_SCSI_EATA_PIO
#undef  CONFIG_SCSI_FUTURE_DOMAIN
#undef  CONFIG_SCSI_GDTH
#undef  CONFIG_SCSI_GENERIC_NCR5380
#undef  CONFIG_SCSI_IPS
#undef  CONFIG_SCSI_INITIO
#undef  CONFIG_SCSI_INIA100
#undef  CONFIG_SCSI_NCR53C406A
#undef  CONFIG_SCSI_NCR53C7xx
#undef  CONFIG_SCSI_NCR53C8XX
#undef  CONFIG_SCSI_SYM53C8XX
#undef  CONFIG_SCSI_PAS16
#undef  CONFIG_SCSI_PCI2000
#undef  CONFIG_SCSI_PCI2220I
#undef  CONFIG_SCSI_PSI240I
#undef  CONFIG_SCSI_QLOGIC_FAS
#undef  CONFIG_SCSI_QLOGIC_ISP
#undef  CONFIG_SCSI_QLOGIC_FC
#undef  CONFIG_SCSI_QLOGIC_1280
#undef  CONFIG_SCSI_SEAGATE
#undef  CONFIG_SCSI_SIM710
#undef  CONFIG_SCSI_SYM53C416
#undef  CONFIG_SCSI_DC390T
#undef  CONFIG_SCSI_T128
#undef  CONFIG_SCSI_U14_34F
#undef  CONFIG_SCSI_ULTRASTOR
#undef  CONFIG_SCSI_DEBUG
/*
 * IEEE 1394 (FireWire) support
 */
#undef  CONFIG_IEEE1394
/*
 * I2O device support
 */
#undef  CONFIG_I2O
/*
 * Network device support
 */
#define CONFIG_NETDEVICES 1
/*
 * ARCnet devices
 */
#undef  CONFIG_ARCNET
#undef  CONFIG_DUMMY
#undef  CONFIG_BONDING
#undef  CONFIG_EQUALIZER
#undef  CONFIG_TUN
#undef  CONFIG_NET_SB1000
/*
 * Ethernet (10 or 100Mbit)
 */
#define CONFIG_NET_ETHERNET 1
#undef  CONFIG_NET_VENDOR_3COM
#undef  CONFIG_LANCE
#undef  CONFIG_NET_VENDOR_SMC
#undef  CONFIG_NET_VENDOR_RACAL
#undef  CONFIG_AT1700
#undef  CONFIG_DEPCA
#undef  CONFIG_HP100
#undef  CONFIG_NET_ISA
#define CONFIG_NET_PCI 1
#undef  CONFIG_PCNET32
#undef  CONFIG_ADAPTEC_STARFIRE
#undef  CONFIG_AC3200
#undef  CONFIG_APRICOT
#undef  CONFIG_CS89x0
#define CONFIG_TULIP 1
#undef  CONFIG_DE4X5
#undef  CONFIG_DGRS
#undef  CONFIG_DM9102
#undef  CONFIG_EEPRO100
#undef  CONFIG_NATSEMI
#undef  CONFIG_NE2K_PCI
#undef  CONFIG_8139TOO
#undef  CONFIG_RTL8129
#undef  CONFIG_SIS900
#undef  CONFIG_EPIC100
#undef  CONFIG_SUNDANCE
#undef  CONFIG_TLAN
#undef  CONFIG_VIA_RHINE
#undef  CONFIG_WINBOND_840
#undef  CONFIG_HAPPYMEAL
#undef  CONFIG_NET_POCKET
/*
 * Ethernet (1000 Mbit)
 */
#undef  CONFIG_ACENIC
#undef  CONFIG_HAMACHI
#undef  CONFIG_YELLOWFIN
#undef  CONFIG_SK98LIN
#undef  CONFIG_FDDI
#undef  CONFIG_HIPPI
#undef  CONFIG_PPP
#undef  CONFIG_SLIP
/*
 * Wireless LAN (non-hamradio)
 */
#undef  CONFIG_NET_RADIO
/*
 * Token Ring devices
 */
#undef  CONFIG_TR
#undef  CONFIG_NET_FC
#undef  CONFIG_RCPCI
#undef  CONFIG_SHAPER
/*
 * Wan interfaces
 */
#undef  CONFIG_WAN
/*
 * Amateur Radio support
 */
#undef  CONFIG_HAMRADIO
/*
 * IrDA (infrared) support
 */
#undef  CONFIG_IRDA
/*
 * ISDN subsystem
 */
#undef  CONFIG_ISDN
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
#undef  CONFIG_JFFS_FS
#undef  CONFIG_CRAMFS
#undef  CONFIG_RAMFS
#undef  CONFIG_ISO9660_FS
#undef  CONFIG_MINIX_FS
#undef  CONFIG_NTFS_FS
#undef  CONFIG_HPFS_FS
#define CONFIG_PROC_FS 1
#undef  CONFIG_DEVFS_FS
#undef  CONFIG_QNX4FS_FS
#undef  CONFIG_ROMFS_FS
#define CONFIG_EXT2_FS 1
#undef  CONFIG_SYSV_FS
#undef  CONFIG_UDF_FS
#undef  CONFIG_UFS_FS
/*
 * Network File Systems
 */
#undef  CONFIG_CODA_FS
#undef  CONFIG_NFS_FS
#undef  CONFIG_NFSD
#undef  CONFIG_SUNRPC
#undef  CONFIG_LOCKD
#undef  CONFIG_SMB_FS
#undef  CONFIG_NCP_FS
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

#define CONFIG_DEBUG_IOVIRT 0
