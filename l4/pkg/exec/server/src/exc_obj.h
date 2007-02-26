#ifndef __L4_EXEC_SERVER_EXC_OBJ_H
#define __L4_EXEC_SERVER_EXC_OBJ_H

#include <l4/sys/types.h>
#include <l4/l4rm/l4rm.h>

#include "exc_obj_psec.h"
#include "exc_obj_hsec.h"
#include "exc_obj_stab.h"
#include "exc_img.h"
#include "dsc.h"

#include "config.h"
#include "refcounted.h"
#include "file.h"

/** Base class for an ELF object (library or binary) */
class exc_obj_t : public dsc_obj_t,
		  public obj_refcounted,
		  public file_t
{
  public:
    exc_obj_t(exc_img_t *img, l4_uint32_t _id);
    virtual ~exc_obj_t();

    inline const char* get_pathname(void)
      { return pathname; }
    inline exc_obj_t** get_deps(void)
      { return deps; }
    inline l4_addr_t get_entry(void)
      { return entry; }
    
    int add_to_env(l4env_infopage_t *env);
    int junk_hsecs(void);

    inline void set_flag(unsigned int flag)
      { flags |= flag; }
    inline void res_flag(unsigned int flag)
      { flags &= ~flag; }
    inline int failed(void)
      { return not_valid; }

    inline void set_client(l4_threadid_t client)
      { client_tid = client; }

    int relocate(l4_addr_t reloc_addr, l4env_infopage_t *env);
    int set_section_type(l4_uint16_t type, l4env_infopage_t *env);
    
    virtual int load_libs(l4env_infopage_t *env) = 0;
    virtual int link(exc_obj_t **deps, l4env_infopage_t *env) = 0;
    virtual int find_sym(const char *symname, l4env_infopage_t *env,
			 l4_addr_t *addr) = 0;
    virtual int get_symbols(l4env_infopage_t *env, char **str) = 0;
    virtual int get_symbols_size(l4_size_t *symsize) = 0;
    virtual int get_lines(l4env_infopage_t *env,
			  char **str, stab_line_t **lin, unsigned str_offs) = 0;
    virtual int get_lines_size(l4_size_t *str, l4_size_t *lin) = 0;
    virtual int img_copy(exc_img_t *img, l4env_infopage_t *env) = 0;

  protected:
    inline exc_obj_psec_t* addr_psec(l4_addr_t addr)
      { return range_psec(addr, 4); }
    
    exc_obj_hsec_t* lookup_hsec(int idx);
    int add_to_dep(exc_obj_t *new_exc_obj);
    
    exc_obj_psec_t* range_psec(l4_addr_t addr, l4_size_t size);
    
    l4_addr_t env_reloc_addr(l4exec_section_t *l4exc, l4env_infopage_t *env);

    /** The object's program entry */
    l4_addr_t entry;

    /** Number of header sections. 
        The header section data is necessary for dynamic linking. */
    int	hsecs_num;
    /** header sections descriptors */
    exc_obj_hsec_t *hsecs;
    
    /** number of program sections */
    int	psecs_num;
    /** program sections descriptors */
    exc_obj_psec_t *psecs[EXC_MAXPSECT];

    /** line numbers information */
    exc_obj_stab_t *stab;

    /** dependant EXC libraries */
    int	deps_num;			/* number of dependant libs */
    exc_obj_t* deps[EXC_MAXDEP+1];	/* dependant libs descriptors */

    /** name of EXC object */
    char pathname[EXC_MAXFNAME];	/* filename (including path) */

    /** number of relocation entries in text segments (should be 0
     * to allow sharing of text segments) */
    int	textreloc_num;

    /** common flags */
    int flags;
#define EO_LIBS_LOADED		0x00000001  /** dependant libraries loaded */
#define EO_DYNAMIC		0x00000002  /** exec object is dynamic */
#define EO_SHARE		0x00000004  /** share sections of exec object */
#define EO_LOAD_SYMBOLS		0x00000008  /** hold symbols */
#define EO_LOAD_LINES		0x00000010  /** hold lines */
#define EO_DIRECT_MAP		0x00000020  /** program sections direct map'd */

    int not_valid;

    /** Id of client which owns the exc_obj. This is the loader. All
     * initially paged sections will be transfered to the application
     * pager. All other sections (shared libraries) will be transfered
     * to the application itself after startup */
    l4_threadid_t client_tid;
};

/** allocate program section id */
int exc_obj_alloc_sect(void);

/** load binary */
int exc_obj_load_bin(const char *fname, int force_load, l4_threadid_t client,
		     int flags, exc_obj_t **exc_obj, l4env_infopage_t *env);

#define SH_ADDR_HERE(sh,ph)  ((ph)->vaddr + (sh)->sh_addr - (ph)->l4exc.addr)

extern dsc_array_t *exc_objs;


#endif /* __L4_EXEC_SERVER_EXC_OBJ_H */

