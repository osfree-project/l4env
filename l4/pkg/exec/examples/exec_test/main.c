/* $Id$ */
/*****************************************************************************
 * exec/examples/exec_test/main.c                                            *
 *                                                                           *
 * Created:   08/20/2000                                                     *
 * Author(s): Frank Mehnert <fm3@os.inf.tu-dresden.de>                       *
 *                                                                           *
 * Common L4 environment                                                     *
 * Example for using the EXEC layer                                          *
 *****************************************************************************/

#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/env/env.h>
#include <l4/l4rm/l4rm.h>
#include <l4/exec/elf.h>
#include <l4/exec/exec.h>
#include <l4/exec/errno.h>
#include <l4/exec/exec-client.h>
#include <l4/dm_mem/dm_mem.h>

#include <stdio.h>


static int
load_app(char *fname, l4_threadid_t exec_id, l4env_infopage_t *env)
{
  int error;
  l4_addr_t sec_beg, sec_end;
  l4_addr_t area_beg, area_end;
  l4_addr_t sec_addr, area_addr;
  l4_size_t sec_size, area_size;
  l4exec_section_t *l4exc;
  l4exec_section_t *l4exc_tmp;
  l4exec_section_t *l4exc_stop;
  l4_uint32_t rm_area;
  sm_exc_t exc;

  /* first stage */
  error = l4exec_bin_open(exec_id, fname, 
 			   (l4exec_envpage_t_slice*)env, 0, &exc);
  if (error  != -L4_EXEC_NOSTANDARD)
    {
      printf("Error %d loading file\n", error);
      return error;
    }

  /* Relocate all sections so that exec can link they */
  l4exc_stop = env->section + env->section_num;
  for (l4exc=env->section; l4exc<l4exc_stop; l4exc++)
    {
      if (l4exc->info.type & L4_DSTYPE_RELOCME)
	{

	  /* This code comes from loader/lib/src/main.c */

	  /* At least this section must be relocated. We can decide, where
	   * it should lay in our address space. With one restriction: The
	   * relative position of sections of the same area must be the
	   * same as in the file image. Therefore we first reserve an free
	   * area for all sections of this ELF object (same area ID) and
	   * then attach all sections to this area */
	  area_beg = L4_MAX_ADDRESS;
	  area_end = 0;

	  for (l4exc_tmp=l4exc; l4exc<l4exc_stop; l4exc++)
	    {
	      sec_beg = l4exc->addr;
	      if (sec_beg < area_beg) area_beg = sec_beg;

	      sec_end = l4exc->size + sec_beg;
	      if (sec_end > area_end) area_end = sec_end;

	      if (l4exc->info.type & L4_DSTYPE_OBJ_END)
		break;
	    }
	 
	  /* align area address and area size */
	  area_beg =  area_beg                    & L4_PAGEMASK;
	  area_end = (area_end + L4_PAGESIZE - 1) & L4_PAGEMASK;
	  area_size = area_end - area_beg;

	  if (area_beg != 0)
	    {
	      printf("Relocatable area starts at %08x\n", area_beg);
	      printf("sections at %p size %08x\n",
		  env->section, sizeof(l4exec_section_t));
	      enter_kdebug("stop");
	    }

	  /* reserve area */
	  if ((error = l4rm_direct_area_reserve(area_size, 0,
						&area_addr, &rm_area)))
	    {
	      printf("Error reserving area (%08x size %08x)\n",
		  area_addr, area_size);
	      enter_kdebug("area_reserve");
	    }

	  /* Now as we know the size of the area we attach to it */
	  for (l4exc=l4exc_tmp; l4exc<l4exc_stop; l4exc++)
	    {
	      /* align section address and section size */
	      sec_beg = l4exc->addr;
	      sec_end = l4exc->size + sec_beg;
	      sec_beg =  sec_beg                    & L4_PAGEMASK;
	      sec_end = (sec_end + L4_PAGESIZE - 1) & L4_PAGEMASK;
	      sec_addr = sec_beg;
	      sec_size = sec_end - sec_beg;

	      /* section offsets are relative to area offset */
	      sec_addr += area_addr;
	     
	      /* Sections have fixed offsets in the region */
	      if ((error =
		    l4rm_direct_area_attach_to_region(&l4exc->ds,
				      rm_area, (void *)sec_addr, sec_size, 0, 0)))
		{
		  printf("Error %d attaching to area id %d (%08x-%08x)\n",
		         error, rm_area, sec_addr, sec_addr+sec_size);
		  l4rm_show_region_list();
		  enter_kdebug("attach_to_region");
		}

	      l4exc->addr = sec_addr;
	      l4exc->info.type &= ~L4_DSTYPE_RELOCME;

	      if (l4exc->info.type & L4_DSTYPE_OBJ_END)
		break;
	    }
	}
    }
  
  /* second stage */
  if ((error = l4exec_bin_link(exec_id, (l4exec_envpage_t_slice*)env, &exc)))
    {
      printf("Error %d linking complete file\n", error);
      return error;
    }

  return 0;
}


int
main(void)
{
  l4dm_dataspace_t ds;
  l4_threadid_t dm_id, tftp_id, exec_id;
  l4env_infopage_t *env;
  
  LOG_init("exectst");

  if (!names_waitfor_name("TFTP", &tftp_id, 5000))
    {
      printf("TFTP not found\n");
      return -1;
    }
  if (!names_waitfor_name("EXEC", &exec_id, 5000))
    {
      printf("EXEC not found\n");
      return -1;
    }
  if (!names_waitfor_name("SIMPLE_DM", &dm_id, 5000))
    {
      printf("SIMPLE_DM not found\n");
      return -1;
    }

  /* get virtual + physical memory for infopage */
  env = l4dm_mem_ds_allocate_named(L4_PAGESIZE,0,"env",&ds);

  /* map in and fill out infopage */
  env->fprov_id    = tftp_id;
  env->image_dm_id = dm_id;
  env->text_dm_id  = dm_id;
  env->data_dm_id  = dm_id;
  env->ver_info.arch_class = ARCH_ELF_ARCH_CLASS;
  env->ver_info.arch_data  = ARCH_ELF_ARCH_DATA;
  env->ver_info.arch       = ARCH_ELF_ARCH;
  env->addr_libloader =  0x0e000;
  env->addr_libl4rm   =  0x10000;

  strcpy(env->binpath, "(nd)/tftpboot/fm3/gimp/");
  strcpy(env->libpath, "(nd)/tftpboot/fm3/gimp/");

  printf("sizeof(l4env_infopage_t) = %d (max %d) dwords\n", 
          (sizeof(l4env_infopage_t) + 
	   sizeof(l4_umword_t) - 1) / sizeof(l4_umword_t),
	  L4_MAX_RPC_BUFFER_SIZE);
 
  load_app("gimp", exec_id, env);
  load_app("gimp", exec_id, env);

  printf("Test Done.\n");
  
  return 0;
}

