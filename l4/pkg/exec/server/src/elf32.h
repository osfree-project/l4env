/*!
 * \file	exec/server/src/elf32.h
 * \brief	elf32 interface
 *
 * \date	2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __ELF32_H_
#define __ELF32_H_

#include <l4/exec/elf.h>

#include "exc.h"
#include "exc_obj.h"

/** Class for ELF32 objects */
class elf32_obj_t: public exc_obj_t
{
  public:
    /** constructor */
    elf32_obj_t(exc_img_t *img, l4_uint32_t _id);
    /** destruktor */
    virtual ~elf32_obj_t();

    /** load all dependant dynmaic libraries */
    int load_libs(l4env_infopage_t *env);

    Elf32_Shdr* img_lookup_sh(int i, exc_img_t *img);
    int link(exc_obj_t **deps, l4env_infopage_t *env);

    /** find the symbol in the ELF object */
    int find_sym(const char *symname, int need_global, Elf32_Sym** sym);
    /** find the symbol in the ELF object */
    int find_sym(const char *symname, l4env_infopage_t *env, l4_addr_t *addr);
    /** Find the relocation addr belonging to addr. */
    int addr_to_reloc_offs(l4_addr_t addr, l4env_infopage_t *env,
			   l4_addr_t *reloc_offs);
    /** Find the relocation address <reloc_addr> the symbol <sym> was relocated
     *  * in the context of the envpage <env>. */
    int sym_to_reloc_offs(Elf32_Sym *sym, const char *strtab,
			  l4env_infopage_t *env, l4_addr_t *reloc_offs);
    /** */
    int sym_vaddr(Elf32_Sym *sym, l4_addr_t *vaddr);

    /** get symbols */
    int get_symbols(l4env_infopage_t *env, char **str);
    int get_symbols_size(l4_size_t *sym);

    /** get lines */
    int get_lines(l4env_infopage_t *env, char **str, stab_line_t **lin, 
		  unsigned str_offs);
    int get_lines_size(l4_size_t *str, l4_size_t *lin);
    
    inline const char* dyn_symbol(Elf32_Word symbol)
      { return dyn_strtab + dyn_symtab[symbol].st_name; }
    
    inline exc_obj_psec_t* sh_psec(Elf32_Shdr *sh)
      { return range_psec(sh->sh_addr, sh->sh_size); }
    
  protected:
    exc_obj_psec_t* sym_psec(Elf32_Sym *sym, const char *strtab);
    
    int img_create_psecs(exc_img_t *img, l4env_infopage_t *env);
    int img_create_hsecs(exc_img_t *img);
    int img_junk_hsecs_on_nodyn(l4env_infopage_t *env);
    int img_save_info(exc_img_t *img);
    int img_fill_psecs(exc_img_t *img);
    int img_save_symbols(exc_img_t *img);
    int img_save_lines(exc_img_t *img);
    int img_copy(exc_img_t *img, l4env_infopage_t *env);

  private:
    int link_sym(Elf32_Rel *rel, 
		 l4_addr_t rel_l4exc_vaddr,
		 exc_obj_psec_t *rel_psec,
		 l4exec_section_t *rel_l4exc,
		 l4_addr_t rel_l4exc_reloc,
		 exc_obj_t **bin_deps,
		 l4env_infopage_t *env);
    int link_entry(l4env_infopage_t *env);

    const char* dyn_strtab;
    Elf32_Word* dyn_hashtab;
    Elf32_Sym* dyn_symtab;
    const char* sym_strtab;
    unsigned int size_strtab;
    Elf32_Sym* sym_symtab;
    unsigned int num_symtab;
};


int elf32_obj_check_ftype(exc_img_t *img, l4env_infopage_t *env, int verbose);
int elf32_obj_check_interp(exc_img_t *img, int verbose);
int elf32_obj_new(exc_img_t *img, exc_obj_t **exc_obj, l4env_infopage_t *env,
		  l4_uint32_t _id);

#endif /* __L4_EXEC_SERVER_ELF32_H */

