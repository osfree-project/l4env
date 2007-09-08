/*
 * \brief   Munich - starts Linux
 * \date    2006-06-28
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2006  Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the OSLO package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include "version.h"
#include "util.h"
#include "munich.h"
#include "boot_linux.h"
#include "mp.h"

const char *message_label = "MUNICH: ";

const unsigned REALMODE_STACK = 0x49000;
const unsigned REALMODE_IMAGE = 0x40000;


/**
 * Starts a linux from multiboot modules. Treats the first module as
 * linux kernel and the optional second module as initrd.
 */
int
start_linux(struct mbi *mbi)
{
  struct module *m  = (struct module *) (mbi->mods_addr);
  struct linux_kernel_header *hdr = (struct linux_kernel_header *)(m->mod_start + 0x1f1);

  // sanity checks
  ERROR(-11, ~mbi->flags & MBI_FLAG_MODS, "module flag missing");
  ERROR(-12, !mbi->mods_count, "no kernel to start");
  ERROR(-13, 2 < mbi->mods_count, "do not know what to do with that many modules");
  ERROR(-14, LINUX_BOOT_FLAG_MAGIC != hdr->boot_flag, "boot flag does not match");
  ERROR(-15, LINUX_HEADER_MAGIC != hdr->header, "too old linux version?");
  ERROR(-16, 0x202 > hdr->version, "can not start linux pre 2.4.0");
  ERROR(-17, !(hdr->loadflags & 0x01), "not a bzImage?");

  // filling out the header
  hdr->type_of_loader = 0x7;      // fake GRUB here
  hdr->cmd_line_ptr   = REALMODE_STACK;
  // output kernel version string
  if (hdr->kernel_version)
    {
      ERROR(-18, hdr->setup_sects << 9 < hdr->kernel_version, "version pointer invalid");
      out_info((char *)(m->mod_start + hdr->kernel_version + 0x200));
    }

  //fix cmdline
  char *cmdline = (char *) m->string;
  while (*cmdline && *cmdline++ !=' ')
    ;
  out_info(cmdline);

  // handle initrd
  if (1 < mbi->mods_count)
    {
      hdr->ramdisk_image = (m+1)->mod_start;
      hdr->ramdisk_size = (m+1)->mod_end - (m+1)->mod_start;

      out_description("initrd",hdr->ramdisk_image);
      ERROR(-19, hdr->ramdisk_image + hdr->ramdisk_size > hdr->initrd_addr_max, "kernel can not reach initrd")
    }


  out_info("copy image");
  memcpy((char *) REALMODE_IMAGE, (char *) m->mod_start, (hdr->setup_sects+1) << 9);
  memcpy((char *) hdr->cmd_line_ptr, cmdline, strlen(cmdline)+1);
  memcpy((char *) hdr->code32_start, 
	 (char *)  m->mod_start + ((hdr->setup_sects+1) << 9),
	 hdr->syssize*16);

  out_info("start kernel");
  jmp_kernel(REALMODE_IMAGE / 16 + 0x20, REALMODE_STACK);
}


/**
 * Start a linux from a multiboot structure.
 */
int
__main(struct mbi *mbi, unsigned flags)
{
#ifndef NDEBUG
  serial_init();
#endif
  out_info(VERSION " starts Linux");
  ERROR(10, !mbi || flags != MBI_MAGIC, "Not loaded via multiboot");


  if (0 <= check_cpuid())
    {
      /**
       * Start the stopped APs and execute some fixup code.
       * Note: we reuse the REALMODE_IMAGE region here.
       */
      memcpy((char *) REALMODE_IMAGE, &smp_init_start, &smp_init_end - &smp_init_start);
      ERROR(26, start_processors(REALMODE_IMAGE), "sending an STARTUP IPI to other processors failed");

      asm volatile("stgi");
    }

  ERROR(11, start_linux(mbi), "start linux failed");
  return 12;
}
