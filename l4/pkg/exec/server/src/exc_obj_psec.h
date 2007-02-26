/*!
 * \file	exc_obj_psec.h
 * \brief	
 *
 * \date	2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __EXC_OBJ_PSEC_H_
#define __EXC_OBJ_PSEC_H_

#include <l4/sys/types.h>
#include <l4/env/env.h>
#include <l4/l4rm/l4rm.h>

#include "refcounted.h"

/** program section descriptor */
class exc_obj_psec_t : public obj_refcounted
{
  public:
    /** virtual address of section in my address space */
    l4_addr_t		vaddr;
    /** linking offset of section in target address space. By subtracting
     * the link_addr from an symbol's address, the offset into the program
     * section is determined. */
    l4_addr_t		link_addr;
    /** L4 execution section */
    l4exec_section_t	l4exc;

    exc_obj_psec_t();
    ~exc_obj_psec_t();
    
    /** init/attach dataspace */
    int init_ds(l4_addr_t addr, l4_size_t size_aligned,
		int direct, int fixed,
		l4_threadid_t dm_id, const char *dbg_name);
    /** junk/detach dataspace */
    int done_ds(void);
    /** get the corresponding section in L4 envpage */
    l4exec_section_t *lookup_env(l4env_infopage_t *env);
    /** share program section */
    int share_to_env(l4env_infopage_t *env, l4exec_section_t **out_l4exc,
		     l4_threadid_t client);
    /** test if [addr,addr+size] are located inside this psec */
    int inline contains(l4_addr_t addr, l4_size_t size);
    /** test if [addr,addr+4] are located inside this psec */
    int inline contains(l4_addr_t addr);
};

int inline
exc_obj_psec_t::contains(l4_addr_t addr, l4_size_t size)
{
  return ((addr >= l4exc.addr) && (addr+size <= l4exc.addr+l4exc.size));
}

int inline
exc_obj_psec_t::contains(l4_addr_t addr)
{
  return ((addr >= l4exc.addr) && (addr+4 <= l4exc.addr+l4exc.size));
}

/** get the address in our address space */
l4_addr_t exc_obj_psec_here(l4env_infopage_t *env, int envsec_idx);

/** kill all program section of environment page */
int exc_obj_psec_kill_from_env(l4env_infopage_t *env);

#endif /* __L4_EXEC_SERVER_ELF_OBJ_PSEC_H */

