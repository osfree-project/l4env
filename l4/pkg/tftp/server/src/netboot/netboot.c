#include <l4/util/rdtsc.h>

#include "etherboot.h"
#include "netboot.h"
#include "filesys.h"

static int block_file = 0;

int filemax;
grub_error_t errnum = ERR_NONE;
unsigned long saved_drive;
unsigned long current_drive = 0xFF;
unsigned long saved_partition;
unsigned long current_partition;
unsigned netboot_buf;
int fsmax;
int fsys_type = NUM_FSYS;
int filepos;
int print_possibilities;
int buf_drive = -1;
char config_file[128] = "<config_file>";

static int incomplete, disk_choice;

static enum
{
  PART_UNSPECIFIED = 0,
  PART_DISK,
  PART_CHOSEN,
}
part_choice;

struct fsys_entry fsys_table[NUM_FSYS + 1] =
{
  /* TFTP should come first because others don't handle net device.  */
  {"tftp", tftp_mount, tftp_read, tftp_dir, tftp_close, 0},
  {0, 0, 0, 0, 0, 0}
};


static void
attempt_mount (void)
{
  for (fsys_type = 0; fsys_type < NUM_FSYS; fsys_type++)
    if ((fsys_table[fsys_type].mount_func) ())
      break;

  if (fsys_type == NUM_FSYS && errnum == ERR_NONE)
    errnum = ERR_FSYS_MOUNT;
}

static int
sane_partition (void)
{
  /* network drive */
  if (current_drive == NETWORK_DRIVE)
    return 1;
  
  if (!(current_partition & 0xFF000000uL)
      && (current_drive & 0xFFFFFF7F) < 8
      && (current_partition & 0xFF) == 0xFF
      && ((current_partition & 0xFF00) == 0xFF00
	  || (current_partition & 0xFF00) < 0x800)
      && ((current_partition >> 16) == 0xFF
	  || (current_drive & 0x80)))
    return 1;

  errnum = ERR_DEV_VALUES;
  return 0;
}

char *
set_device (char *device)
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
	  if (*device == 'f' || *device == 'h' ||
	      (*device == 'n' && network_ready))
	    {
	      /* user has given '([fhn]', check for resp. add 'd' and
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

	  if ((*device == 'f' || *device == 'h' ||
	       (*device == 'n' && network_ready))
	      && (device += 2, (*(device - 1) != 'd')))
	    errnum = ERR_NUMBER_PARSING;

	  if (ch == 'n' && network_ready)
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
	  part_choice ++;
	  device++;

	  if (*device >= '0' && *device <= '9')
	    {
	      part_choice ++;
	      current_partition = 0;

	      if (!(current_drive & 0x80)
		  || !safe_parse_maxint (&device, (int *) &current_partition)
		  || current_partition > 254)
		{
		  errnum = ERR_DEV_FORMAT;
		  return 0;
		}

	      current_partition = (current_partition << 16) + 0xFFFF;

	      if (*device == ',')
		device++;
	      
	      if (*device >= 'a' && *device <= 'h')
		{
		  current_partition = (((*(device++) - 'a') << 8)
				       | (current_partition & 0xFF00FF));
		}
	    }
	  else if (*device >= 'a' && *device <= 'h')
	    {
	      part_choice ++;
	      current_partition = ((*(device++) - 'a') << 8) | 0xFF00FF;
	    }

	  if (*device == ')')
	    {
	      if (part_choice == PART_DISK)
		{
		  current_partition = saved_partition;
		  part_choice ++;
		}

	      result = 1;
	    }
	}
    }

  if (! sane_partition ())
    return 0;
  
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

/*
 *  This performs a "mount" on the current device, both drive and partition
 *  number. 
 */
int
open_device (void)
{
  attempt_mount ();

  if (errnum != ERR_NONE)
    return 0;

  return 1;
}

static char *
setup_part (char *filename)
{
  if (*filename == '(')
    {
      if ((filename = set_device (filename)) == 0)
	{
	  current_drive = 0xFF;
	  return 0;
	}
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
      open_device ();
    }
  
  if (errnum && (*filename == '/' || errnum != ERR_FSYS_MOUNT))
    return 0;
  else
    errnum = 0;

  return filename;
}


int
netboot_open (char *filename)
{
  compressed_file = 0;
  filepos = 0;
  errnum = ERR_NONE;

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
netboot_read (unsigned char *buf, int len)
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
//  eth_disable();

  if (fsys_table[fsys_type].close_func != 0)
    (*(fsys_table[fsys_type].close_func)) ();
}

int
netboot_init (unsigned bufaddr, unsigned gunzipaddr)
{
  extern unsigned gunzip_buf;

  l4_calibrate_tsc ();
  gunzip_buf = gunzipaddr;
  netboot_buf = bufaddr;
  dhcp();
  print_network_configuration ();
  return 0;
}

void
netboot_set_server (in_addr server_addr)
{
  arptable[ARP_SERVER].ipaddr.s_addr = server_addr.s_addr;

  /* Kill arp.  */
  grub_memset (arptable[ARP_SERVER].node, 0, ETH_ALEN);

  print_network_configuration ();
}

void
netboot_show_drivers (void)
{
  int col, comma;

  printf("NICs supported: ");
  col   = 16;
  comma = 0;
#ifdef CONFIG_ORE
  printf("ORe");
  comma = 1;
  col += 3;
#endif
  putchar('\n');
}

char *err_list[] =
{
  [ERR_NONE] = 0,
  [ERR_BAD_ARGUMENT] = "Invalid argument",
  [ERR_BAD_FILENAME] =
  "Filename must be either an absolute pathname or blocklist",
  [ERR_BAD_FILETYPE] = "Bad file or directory type",
  [ERR_BAD_GZIP_DATA] = "Bad or corrupt data while decompressing file",
  [ERR_BAD_GZIP_HEADER] = "Bad or incompatible header in compressed file",
  [ERR_BAD_PART_TABLE] = "Partition table invalid or corrupt",
  [ERR_BAD_VERSION] = "Mismatched or corrupt version of stage1/stage2",
  [ERR_BELOW_1MB] = "Loading below 1MB is not supported",
  [ERR_BOOT_COMMAND] = "Kernel must be loaded before booting",
  [ERR_BOOT_FAILURE] = "Unknown boot failure",
  [ERR_BOOT_FEATURES] = "Unsupported Multiboot features requested",
  [ERR_DEV_FORMAT] = "Unrecognized device string",
  [ERR_DEV_NEED_INIT] = "Device not initialized yet",
  [ERR_DEV_VALUES] = "Invalid device requested",
  [ERR_EXEC_FORMAT] = "Invalid or unsupported executable format",
  [ERR_FILELENGTH] =
  "Filesystem compatibility error, cannot read whole file",
  [ERR_FILE_NOT_FOUND] = "File not found",
  [ERR_FSYS_CORRUPT] = "Inconsistent filesystem structure",
  [ERR_FSYS_MOUNT] = "Cannot mount selected partition",
  [ERR_GEOM] = "Selected cylinder exceeds maximum supported by BIOS",
  [ERR_NEED_LX_KERNEL] = "Linux kernel must be loaded before initrd",
  [ERR_NEED_MB_KERNEL] = "Multiboot kernel must be loaded before modules",
  [ERR_NEED_SERIAL] = "Serial device not configured",
  [ERR_NO_DISK] = "Selected disk does not exist",
  [ERR_NO_DISK_SPACE] = "No spare sectors on the disk",
  [ERR_NO_PART] = "No such partition",
  [ERR_NUMBER_OVERFLOW] = "Overflow while parsing number",
  [ERR_NUMBER_PARSING] = "Error while parsing number",
  [ERR_OUTSIDE_PART] = "Attempt to access block outside partition",
  [ERR_PRIVILEGED] = "Must be authenticated",
  [ERR_READ] = "Disk read error",
  [ERR_SYMLINK_LOOP] = "Too many symbolic links",
  [ERR_UNALIGNED] = "File is not sector aligned",
  [ERR_UNRECOGNIZED] = "Unrecognized command",
  [ERR_WONT_FIT] = "Selected item cannot fit into memory",
  [ERR_WRITE] = "Disk write error",
  [ERR_BADMODADDR] = "Bad modaddr",
};

