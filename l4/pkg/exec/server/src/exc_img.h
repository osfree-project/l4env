#ifndef __L4_EXEC_SERVER_EXC_IMG_H
#define __L4_EXEC_SERVER_EXC_IMG_H

#include <l4/sys/types.h>
#include <l4/env/env.h>

#include "config.h"
#include "file.h"

/** Struct for an ELF image */
class exc_img_t : public file_t 
{
  public:
    exc_img_t(const char *fname, l4env_infopage_t *env);
    ~exc_img_t();
    
    inline const char *get_pathname(void)
      { return pathname; }
    void set_pathname(const char *pathname, l4env_infopage_t *env);

    /** Request image from file provider */
    int load(l4env_infopage_t *env);
   
    /** Virtual address the ELF image is mapped in */
    char		*vaddr;
    /** Size of ELF image */
    l4_size_t		size;

  protected:
    /** Full pathname of ELF object */
    char		pathname[EXC_MAXFNAME];
};

#endif /* __L4_EXEC_SERVER_EXC_IMG_H */

