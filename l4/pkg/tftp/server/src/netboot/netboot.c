/*
 * Most of this code was cut 'n pasted from GNU GRUB
 */

#include "netboot.h"
#include "etherboot.h"
#include "filesys.h"
#include "pci.h"
#include "nic.h"
#include "cards.h"

#include "currticks.h"
#include "grub_emu.h"

#include <l4/util/rdtsc.h>
#include <l4/pci/libpci.h>
#include <l4/sys/kdebug.h>

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "../tftp_config.h"

netboot_error_t errnum = ERR_NONE;
unsigned int netboot_buf = 0;

struct multiboot_info mbi;

int buf_drive = -1;
int print_possibilities;

void (*disk_read_hook) (int, int, int) = NULL;
void (*disk_read_func) (int, int, int) = NULL;

unsigned long saved_drive;
unsigned long saved_partition;
unsigned long current_partition;

static unsigned int pci_ioaddr = 0;
static unsigned short pci_ioaddrs[16];

static int block_file = 0;

int fsmax;
int fsys_type = NUM_FSYS;

struct fsys_entry fsys_table[NUM_FSYS + 1] =
{
  {"tftp", tftp_mount, tftp_read, tftp_dir, tftp_close, 0},
  {0, 0, 0, 0, 0, 0}
};


const char *kernel;
char kernel_buf[128];

#if CONFIG_TULIP
static struct pci_device
  pci_nic_list_tulip[] =
{
  {PCI_VENDOR_ID_DEC, PCI_DEVICE_ID_DEC_TULIP,
   "Digital Tulip", 0, 0, 0},
  {PCI_VENDOR_ID_DEC, PCI_DEVICE_ID_DEC_TULIP_FAST,
   "Digital Tulip Fast", 0, 0, 0},
  {PCI_VENDOR_ID_DEC, PCI_DEVICE_ID_DEC_TULIP_PLUS,
   "Digital Tulip+", 0, 0, 0},
  {PCI_VENDOR_ID_DEC, PCI_DEVICE_ID_DEC_21142,
   "Digital Tulip 21142", 0, 0, 0},
  {PCI_VENDOR_ID_MACRONIX, PCI_DEVICE_ID_MACRONIX_MX987x5,
   "Macronix MX987x5", 0, 0, 0},
  {PCI_VENDOR_ID_LINKSYS, PCI_DEVICE_ID_LINKSYS_LC82C115,
   "LinkSys LNE100TX", 0, 0, 0},
  {PCI_VENDOR_ID_LINKSYS, PCI_DEVICE_ID_DEC_TULIP,
   "Netgear FA310TX", 0, 0, 0},
  {PCI_VENDOR_ID_DAVICOM, PCI_DEVICE_ID_DAVICOM_DM9102,
   "Davicom 9102", 0, 0, 0},
  {0, 0, NULL, 0, 0, 0}
};
#endif

#if CONFIG_EEPRO100
static struct pci_device
  pci_nic_list_eepro100[] =
{
  { PCI_VENDOR_ID_INTEL, PCI_DEVICE_ID_INTEL_82557,
    "Intel EtherExpressPro100", 0, 0, 0},
  {0, 0, NULL, 0, 0, 0}
};
#endif

#if CONFIG_PCNET32
static struct pci_device
  pci_nic_list_pcnet32[] =
{
  { PCI_VENDOR_ID_AMD, PCI_DEVICE_ID_AMD_LANCE,
    "PCNET32/PCI", 0, 0, 0 },
  {0, 0, NULL, 0, 0, 0}
};
#endif

#if CONFIG_NE2000
static struct pci_device
  pci_nic_list_nepci[] =
{
  { PCI_VENDOR_ID_REALTEK, PCI_DEVICE_ID_REALTEK_8029,
    "Realtek 8029", 0, 0, 0},
  { PCI_VENDOR_ID_WINBOND2, PCI_DEVICE_ID_WINBOND2_89C940,
    "Winbond NE2000-PCI", 0, 0, 0},
  { PCI_VENDOR_ID_COMPEX, PCI_DEVICE_ID_COMPEX_RL2000,
    "Compex ReadyLink 2000", 0, 0, 0},
  { PCI_VENDOR_ID_KTI, PCI_DEVICE_ID_KTI_ET32P2,
    "KTI ET32P2", 0, 0, 0},
  { PCI_VENDOR_ID_NETVIN, PCI_DEVICE_ID_NETVIN_NV5000SC,
    "NetVin NV5000SC", 0, 0, 0},
  {0, 0, NULL, 0, 0, 0}
};
#endif

#if CONFIG_3C595
static struct pci_device
  pci_nic_list_3c595[] =
{
  { PCI_VENDOR_ID_3COM, PCI_DEVICE_ID_3COM_3C590,
    "3Com590", 0, 0, 0},
  { PCI_VENDOR_ID_3COM, PCI_DEVICE_ID_3COM_3C595TX,
    "3Com595", 0, 0, 0},
  { PCI_VENDOR_ID_3COM, PCI_DEVICE_ID_3COM_3C595T4,
    "3Com595", 0, 0, 0},
  { PCI_VENDOR_ID_3COM, PCI_DEVICE_ID_3COM_3C595MII,
    "3Com595", 0, 0, 0},
  {0, 0, NULL, 0, 0, 0}
};
#endif

#if CONFIG_3C90X
static struct pci_device
  pci_nic_list_3c90x[] =
{
  { PCI_VENDOR_ID_3COM, PCI_DEVICE_ID_3COM_3C900TPO,
    "3COM 3c905", 0, 0, 0},
  { PCI_VENDOR_ID_3COM, PCI_DEVICE_ID_3COM_3C900COMBO,
    "3COM 3c905", 0, 0, 0},
  { PCI_VENDOR_ID_3COM, PCI_DEVICE_ID_3COM_3C905TX,
    "3COM 3c905", 0, 0, 0},
  { PCI_VENDOR_ID_3COM, PCI_DEVICE_ID_3COM_3C905T4,
    "3COM 3c905", 0, 0, 0},
  { PCI_VENDOR_ID_3COM, PCI_DEVICE_ID_3COM_3C905B_TX,
    "3COM 3c905", 0, 0, 0},
  {0, 0, NULL, 0, 0, 0}
};
#endif

#if CONFIG_RTL8139
static struct pci_device
  pic_nic_list_rtl8139[] =
{
  { PCI_VENDOR_ID_REALTEK, PCI_DEVICE_ID_REALTEK_8139,
    "Realtek 8139", 0, 0, 0},
  { PCI_VENDOR_ID_SMC2, PCI_DEVICE_ID_SMC2_1211TX,
    "SMC EZ10/100", 0, 0, 0},
  {0, 0, NULL, 0, 0, 0}
};
#endif

struct dispatch_table
  {
    char *nic_name;
    struct nic *(*eth_probe) (struct nic *, unsigned short *,
			      struct pci_device *);
    unsigned short *probe_ioaddrs;
    struct pci_device *pci_nic_list;
  };


static struct dispatch_table NIC[] =
{
#if CONFIG_OSHKOSH
  {"OshKosh", oshkosh_probe, pci_ioaddrs, 0},
#endif
#if CONFIG_TULIP
  {"Tulip", tulip_probe, pci_ioaddrs, pci_nic_list_tulip},
#endif
#if CONFIG_EEPRO100
  {"EEpro100", eepro100_probe, pci_ioaddrs, pci_nic_list_eepro100},
#endif
#if CONFIG_PCNET32
  {"PCNet32", pcnet32_probe, pci_ioaddrs, pci_nic_list_pcnet32},
#endif
#if CONFIG_3C90X
  {"3c90x", a3c90x_probe, pci_ioaddrs, pci_nic_list_3c90x},
#endif
#if CONFIG_3C595
  {"3c595", t595_probe, pci_ioaddrs, pci_nic_list_3c595},
#endif
#if CONFIG_NE2000
  {"NE2000/PCI", nepci_probe, pci_ioaddrs, pci_nic_list_nepci},
  {"NE*000", ne_probe, 0, 0},
#warning NE
#endif
#if CONFIG_3C509
  {"3c509", t509_probe, 0, 0},
#endif
#if CONFIG_RTL8139
  {"RTL8139", rtl8139_probe, pci_ioaddrs, pic_nic_list_rtl8139},
#endif
  {0, 0, 0}
};


char *err_list[] =
{
  [ERR_NONE] = 0,
  [ERR_BAD_FILENAME] =
  "Bad filename (must be absolute pathname or blocklist)",
  [ERR_BAD_FILETYPE] = "Bad file or directory type",
  [ERR_BAD_GZIP_DATA] = "Bad or corrupt data while decompressing file",
  [ERR_BAD_GZIP_HEADER] = "Bad or incompatible header on compressed file",
  [ERR_BAD_PART_TABLE] = "Partition table invalid or corrupt",
  [ERR_BAD_VERSION] = "Bad or corrupt version of stage1/stage2",
  [ERR_BELOW_1MB] = "Loading below 1MB is not supported",
  [ERR_BOOT_COMMAND] = "Cannot boot without kernel loaded",
  [ERR_BOOT_FAILURE] = "Unknown boot failure",
  [ERR_BOOT_FEATURES] = "Unsupported Multiboot features requested",
  [ERR_DEV_FORMAT] = "Device string unrecognizable",
  [ERR_DEV_VALUES] = "Invalid device requested",
  [ERR_EXEC_FORMAT] = "Invalid or unsupported executable format",
  [ERR_FILELENGTH] =
  "Filesystem compatibility error, cannot read whole file",
  [ERR_FILE_NOT_FOUND] = "File not found",
  [ERR_FSYS_CORRUPT] = "Inconsistent filesystem structure",
  [ERR_FSYS_MOUNT] = "Cannot mount selected partition",
  [ERR_GEOM] = "Selected cylinder exceeds maximum supported by BIOS",
  [ERR_NEED_LX_KERNEL] = "Must load Linux kernel before initrd",
  [ERR_NEED_MB_KERNEL] = "Must load Multiboot kernel before modules",
  [ERR_NO_DISK] = "Selected disk does not exist",
  [ERR_NO_PART] = "No such partition",
  [ERR_NUMBER_PARSING] = "Error while parsing number",
  [ERR_OUTSIDE_PART] = "Attempt to access block outside partition",
  [ERR_READ] = "Disk read error",
  [ERR_SYMLINK_LOOP] = "Too many symbolic links",
  [ERR_UNRECOGNIZED] = "Unrecognized command",
  [ERR_WONT_FIT] = "Selected item cannot fit into memory",
  [ERR_WRITE] = "Disk write error",
  [ERR_BAD_ARGUMENT] = "Invaild argument specified",
  [ERR_UNALIGNED] = "File is not sector aligned",
  [ERR_PRIVILEGED] = "Must be authenticated",
};


static int
eth_dummy (struct nic *nic)
{
  return (0);
}


int
eth_poll (void)
{
  return ((*nic.poll) (&nic));
}


void
eth_transmit (const char *d, unsigned int t, unsigned int s, const void *p)
{
  (*nic.transmit) (&nic, d, t, s, p);
//  twiddle ();
}


void
eth_disable (void)
{
  (*nic.disable) (&nic);
}


void
eth_enable (void)
{
  (*nic.enable) (&nic);
}


int
nul_terminate (char *str)
{
  int ch;

  while (*str && !isspace (*str))
    str++;

  ch = *str;
  *str = 0;
  return ch;
}


void
twiddle ()
{
  static unsigned long lastticks = 0;
  static int count = 0;
  static char tiddles[] = "-\\|/";
  unsigned long ticks;
  if ((ticks = currticks ()) == lastticks)
    return;
  lastticks = ticks;
  putchar (tiddles[(count++) & 3]);
  putchar ('\b');
}


int
getdec (char **ptr)
{
  char *p = *ptr;
  int ret = 0;
  if ((*p < '0') || (*p > '9'))
    return (-1);
  while ((*p >= '0') && (*p <= '9'))
    {
      ret = ret * 10 + (*p - '0');
      p++;
    }
  *ptr = p;
  return (ret);
}


char packet[ETH_MAX_PACKET];
struct arptable_t arptable[MAX_ARP];

struct nic nic =
{
  (void (*)(struct nic *)) eth_dummy,	/* reset */
  eth_dummy,			/* poll */
  (void (*)(struct nic *, const char *,
	    unsigned int, unsigned int,
	    const char *)) eth_dummy,	/* transmit */
  (void (*)(struct nic *)) eth_dummy,	/* disable */
  (void (*)(struct nic *)) eth_dummy,	/* enable */
  0,				/* no aui */
  arptable[ARP_CLIENT].node,	/* node_addr */
  packet,			/* packet */
  0,				/* packetlen */
  0				/* priv_data */
};


static void
scan_bus (struct pci_device *pcidev)
{
  unsigned int devfn, l, bus, buses;
  unsigned char hdr_type = 0;
  unsigned short vendor, device;
  unsigned int membase, ioaddr, romaddr;
  unsigned char class, subclass;
  int i, reg;

  pci_ioaddr = 0;
  buses = 1;
  for (bus = 0; bus < buses; ++bus)
    {
      for (devfn = 0; devfn < 0xff; ++devfn)
	{
	  if (PCI_FUNC (devfn) == 0)
	    pcibios_read_config_byte (bus, devfn, PCI_HEADER_TYPE, &hdr_type);
	  else if (!(hdr_type & 0x80))	/* not a multi-function device */
	    continue;
	  pcibios_read_config_dword (bus, devfn, PCI_VENDOR_ID, &l);
	  /* some broken boards return 0 if a slot is empty: */
	  if (l == 0xffffffff || l == 0x00000000)
	    {
	      hdr_type = 0;
	      continue;
	    }
	  vendor = l & 0xffff;
	  device = (l >> 16) & 0xffff;

	  /* check for pci-pci bridge devices!! - more buses when found */
	  pcibios_read_config_byte (bus, devfn, PCI_CLASS_CODE, &class);
	  pcibios_read_config_byte (bus, devfn, PCI_SUBCLASS_CODE, &subclass);
	  if (class == 0x06 && subclass == 0x04)
	    buses++;

	  for (i = 0; pcidev[i].vendor != 0; i++)
	    {
	      if (   vendor != pcidev[i].vendor
		  || device != pcidev[i].dev_id)
		continue;
	      for (reg = PCI_BASE_ADDRESS_0; 
		   reg <= PCI_BASE_ADDRESS_5; reg += 4)
		{
		  pcibios_read_config_dword (bus, devfn, reg, &ioaddr);

		  if (   (ioaddr & PCI_BASE_ADDRESS_IO_MASK) == 0 
		      || (ioaddr & PCI_BASE_ADDRESS_SPACE_IO) == 0)
		    continue;
		  /* Strip the I/O address out of the returned value */
		  ioaddr &= PCI_BASE_ADDRESS_IO_MASK;
		  /* Get the memory base address */
		  pcibios_read_config_dword (bus, devfn,
					     PCI_BASE_ADDRESS_1, &membase);
		  /* Get the ROM base address */
		  pcibios_read_config_dword (bus, devfn, PCI_ROM_ADDRESS, 
					     &romaddr);
		  romaddr >>= 10;
		  if (!romaddr)
		    printf ("Found %s at 0x%x, no ROM\n",
			    pcidev[i].name, ioaddr);
		  else
		    printf ("Found %s at 0x%x, ROM address 0x%X\n",
			    pcidev[i].name, ioaddr, romaddr);
		  /* Take the first one or the one that matches in boot 
		   * ROM address */
		  if (   (pci_ioaddr == 0)
		      || (romaddr == ((unsigned long) rom.rom_segment << 4)))
		    {
		      pcidev[i].membase = membase;
		      pcidev[i].ioaddr = ioaddr;
		      pcidev[i].devfn = devfn;
		      pcidev[i].bus = bus;
		      return;
		    }
		}
	    }
	}
    }
}


int
safe_parse_maxint (char **str_ptr, int *myint_ptr)
{
  char *ptr = *str_ptr;
  int myint = 0;
  int mult = 10, found = 0;

  if (*ptr == '0' && tolower (*(ptr + 1)) == 'x')
    {
      ptr += 2;
      mult = 16;
    }

  while (1)
    {
      unsigned int digit;

      digit = tolower (*ptr) - '0';
      if (digit > 9)
	{
	  digit -= 'a' - '0';
	  if (mult == 10 || digit > 5)
	    break;
	  digit += 10;
	}

      found = 1;
      if (myint > ((MAXINT - digit) / mult))
	{
	  errnum = ERR_NUMBER_PARSING;
	  return 0;
	}
      myint = (myint * mult) + digit;
      ptr++;
    }

  if (!found)
    {
      errnum = ERR_NUMBER_PARSING;
      return 0;
    }

  *str_ptr = ptr;
  *myint_ptr = myint;

  return 1;
}


static int disk_choice;
int incomplete;

static enum
  {
    PART_UNSPECIFIED = 0,
    PART_DISK,
    PART_CHOSEN,
  }
part_choice;


int
open_device (void)
{
  for (fsys_type = 0; fsys_type < NUM_FSYS
       && (*(fsys_table[fsys_type].mount_func)) () != 1; fsys_type++);

  if (fsys_type == NUM_FSYS && errnum == ERR_NONE)
    errnum = ERR_FSYS_MOUNT;

  if (errnum != ERR_NONE)
    return 0;

  return 1;
}


static char *
netboot_set_device (char *device)
{
  int result = 0;

  incomplete = 0;
  disk_choice = 1;
  part_choice = PART_UNSPECIFIED;
  current_drive = saved_drive;
  current_partition = 0xFFFFFF;

  if (*device == '(' && !*(device + 1))
    /* user has given '(' only, let disk_choice handle what disks we have */
    return device + 1;

  if (*device == '(' && *(++device))
    {
      if (*device != ',' && *device != ')')
	{
	  char ch = *device;

	  if (*device == 'f' || *device == 'h')
	    {
	      /* user has given '([fh]', check for resp. add 'd' and
	         let disk_choice handle what disks we have */
	      if (!*(device + 1))
		{
		  device++;
		  *device++ = 'd';
		  *device = '\0';
		  return device;
		}
	      else if (*(device + 1) == 'd' && !*(device + 2))
		return device + 2;
	    }

	  if ((*device == 'f' || *device == 'h' || *device == 'n')
	      && (device += 2, (*(device - 1) != 'd')))
	    errnum = ERR_NUMBER_PARSING;

	  if (ch == 'n')
	    current_drive = NETWORK_DRIVE;
	  else
	    {
	      safe_parse_maxint (&device, (int *) &current_drive);

	      disk_choice = 0;
	      if (ch == 'h')
		current_drive += 0x80;
	    }
	}

      if (errnum)
	return 0;

      if (*device == ')')
	{
	  part_choice = PART_CHOSEN;
	  result = 1;
	}
      else if (*device == ',')
	{
	  /* Either an absolute PC or BSD partition. */
	  disk_choice = 0;
	  part_choice++;
	  device++;

	  if (*device >= '0' && *device <= '9')
	    {
	      part_choice++;
	      current_partition = 0;

	      if (!(current_drive & 0x80)
		  || !safe_parse_maxint (&device, (int *) &current_partition)
		  || current_partition > 254)
		{
		  errnum = ERR_DEV_FORMAT;
		  return 0;
		}

	      current_partition = (current_partition << 16) + 0xFFFF;

	      if (*device == ','
		  && *(device + 1) >= 'a' && *(device + 1) <= 'h')
		{
		  device++;
		  current_partition = (((*(device++) - 'a') << 8)
				       | (current_partition & 0xFF00FF));
		}
	    }
	  else if (*device >= 'a' && *device <= 'h')
	    {
	      part_choice++;
	      current_partition = ((*(device++) - 'a') << 8) | 0xFF00FF;
	    }

	  if (*device == ')')
	    {
	      if (part_choice == PART_DISK)
		{
		  current_partition = saved_partition;
		  part_choice++;
		}

	      result = 1;
	    }
	}
    }

  if (result)
    return device + 1;
  else
    {
      if (!*device)
	incomplete = 1;
      errnum = ERR_DEV_FORMAT;
    }

  return 0;
}


static char *
setup_part (char *filename)
{
  if (*filename == '(')
    {
      if ((filename = netboot_set_device (filename)) == 0)
	{
	  current_drive = 0xFF;
	  return 0;
	}
      if (*filename == '/')
	open_device ();
    }
  else if (saved_drive != current_drive
	   || saved_partition != current_partition
	   || (*filename == '/' && fsys_type == NUM_FSYS)
	   || buf_drive == -1)
    {
      current_drive = saved_drive;
      current_partition = saved_partition;
      /* allow for the error case of "no filesystem" after the partition
         is found.  This makes block files work fine on no filesystem */
      if (*filename == '/')
	open_device ();
    }


  if (errnum && (*filename == '/' || errnum != ERR_FSYS_MOUNT))
    return 0;
  else
    errnum = ERR_NONE;

  return filename;
}


int
netboot_seek (int offset)
{
  if (offset > filemax || offset < 0)
    return -1;

  filepos = offset;
  return offset;
}


int
netboot_dir (char *dirname)
{
  compressed_file = 0;

  if (!(dirname = setup_part (dirname)))
    return 0;

  if (*dirname != '/')
    errnum = ERR_BAD_FILENAME;

  if (fsys_type == NUM_FSYS)
    errnum = ERR_FSYS_MOUNT;

  if (errnum)
    return 0;

  /* set "dir" function to list completions */
  print_possibilities = 1;

  return (*(fsys_table[fsys_type].dir_func)) (dirname);
}


int
netboot_open (char *filename)
{
  errnum = ERR_NONE;
  
  eth_enable();

  compressed_file = 0;

  filepos = 0;

  if (!(filename = setup_part (filename)))
    return 0;

  block_file = 0;

  /* This accounts for partial filesystem implementations. */
  fsmax = MAXINT;

  if (!errnum && fsys_type == NUM_FSYS)
    errnum = ERR_FSYS_MOUNT;

  /* set "dir" function to open a file */
  print_possibilities = 0;

  if (!errnum && (*(fsys_table[fsys_type].dir_func)) (filename))
    return gunzip_test_header ();

  return 0;
}


int
netboot_read (char *buf, int len)
{
  /* Make sure "filepos" is a sane value */
  if ((filepos < 0) || (filepos > filemax))
    filepos = filemax;

  /* Make sure "len" is a sane value */
  if ((len < 0) || (len > (filemax - filepos)))
    len = filemax - filepos;

  /* if target file position is past the end of
     the supported/configured filesize, then
     there is an error */
  if (filepos + len > fsmax)
    {
      errnum = ERR_FILELENGTH;
      return 0;
    }

  if (compressed_file)
    return gunzip_read (buf, len);

  if (fsys_type == NUM_FSYS)
    {
      errnum = ERR_FSYS_MOUNT;
      return 0;
    }

  return (*(fsys_table[fsys_type].read_func)) (buf, len);
}


void
netboot_close (void)
{
  if (block_file)
    return;

  eth_disable();

  if (fsys_table[fsys_type].close_func != 0)
    (*(fsys_table[fsys_type].close_func)) ();
}


void
netboot_media_change (void)
{
  buf_drive = -1;
}


int
netboot_init (unsigned int bufaddr, unsigned int gunzipaddr)
{
  int found = 0, pci_card = 0;
  struct pci_device *p = NULL;
  struct dispatch_table *t;
  extern int gunzip_buf;

  l4_calibrate_tsc ();
  pcibios_init ();

  netboot_buf = bufaddr;
  gunzip_buf = gunzipaddr;

  for (t = NIC; t->nic_name != 0; t++)
    {
      if (t->pci_nic_list)
	{
	  scan_bus (t->pci_nic_list);

	  for (p = t->pci_nic_list; p->vendor != 0; p++)
	    {
	      pci_card = 0;
	      
	      if (p->ioaddr != 0)
		{
		  pci_ioaddrs[0] = p->ioaddr;
		  pci_card = 1;
		  break;
		}
	    }

	  if (pci_card)
	    {
	      if ((*t->eth_probe) (&nic, t->probe_ioaddrs, p))
		{
		  found = 1;
		  break;
		}
	    }
	}
      else
	{
	  if ((*t->eth_probe) (&nic, t->probe_ioaddrs, p))
	    {
	      found = 1;
	      break;
	    }
	}
    }

  if (!found)
    return -1;

  bootp ();
  print_network_configuration ();

  eth_disable();
  
  return 0;
}
