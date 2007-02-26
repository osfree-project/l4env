/*!
 * \file	exec/server/src/bin_obj.h
 * \brief	Binary Object declaration
 *
 * \date	2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __BIN_OBJ_H_
#define __BIN_OBJ_H_

#include <l4/env/env.h>
#include <l4/sys/consts.h>

#include "exc_obj.h"
#include "dsc.h"

/** Struct for an Binary. A binary object consists of one main binary and
 * no, one or more dependant shared libraries. */
class bin_obj_t : public dsc_obj_t,
		  public file_t
{
  public:
    bin_obj_t(l4_uint32_t _id);
    ~bin_obj_t();

    inline int set_bin(exc_obj_t *exc_obj)
      { set_fname(exc_obj->get_fname());
	return add_to_dep(exc_obj); }
    inline int get_entry(void)
      { return deps[0] ? deps[0]->get_entry() : L4_MAX_ADDRESS; }
    inline int set_entry_point(l4env_infopage_t *env)
      { 
	env->entry_1st = L4_MAX_ADDRESS; 
	env->entry_2nd = get_entry();
	return 0;
      }

    int load_libs(l4env_infopage_t *env);
    int link_first(l4env_infopage_t *env);
    int mark_startup_library(l4env_infopage_t *env);
    int link(l4env_infopage_t *env);
    int get_symbols(l4env_infopage_t *env, l4dm_dataspace_t *sym_ds);
    int get_lines(l4env_infopage_t *env, l4dm_dataspace_t *lines_ds);
    int set_1st_entry(l4env_infopage_t *env);
    int set_2nd_entry(l4env_infopage_t *env);
    int have_dep(exc_obj_t *exc_obj);
    int find_sym(const char *fname, const char *symname,
		 l4env_infopage_t *env, l4_addr_t *addr);
    int check_relocated(l4env_infopage_t *env);

  protected:
    exc_obj_t* find_exc_obj(const char *fname);
    exc_obj_t* get_nextdep(void);
    int add_to_dep(exc_obj_t *exc_obj);

  private:
    exc_obj_t **free_dep;		/**< number of dependant libs */
    exc_obj_t **next_dep;		/**< number of scanned libs */
    exc_obj_t *deps[EXC_MAXLIB+2];	/**< 1st  entry: binary
					 * 2nd+ entry: dependant libraries
					 * last+1 entry: NULL */
};

extern dsc_array_t *bin_objs;		/**< array of all bin_objs */

#endif /* __L4_EXEC_SERVER_BIN_OBJ_H */

