/*!
 * \file	exc_img.h
 * \brief	
 *
 * \date	2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __EXC_IMG_H_
#define __EXC_IMG_H_

#include <l4/sys/types.h>
#include <l4/env/env.h>

#include "config.h"
#include "file.h"

/** Struct for an ELF image */
class exc_img_t : public file_t 
{
  public:
    exc_img_t(const char *fname, l4dm_dataspace_t *ds, int sticky,
	      l4env_infopage_t *env);
    ~exc_img_t();
   
    /** Get pathname of image file */
    inline const char *get_pathname(void)
      { return pathname; }
    /** Set pathname of image file */
    void set_names(const char *pathname, l4env_infopage_t *env);

    /** Request image from file provider */
    int load(l4env_infopage_t *env);
   
    /** Virtual address the ELF image is mapped in */
    char		*vaddr;
    /** Size of ELF image */
    l4_size_t		size;
    /** Dataspace of image. Only used if dataspace is passed by open 
     * function */
    l4dm_dataspace_t    img_ds;

  protected:
    /** Full pathname of ELF object, not known until successfully loaded */
    char		pathname[EXC_MAXFNAME];

    /** kill of dataspace if set */
    int			sticky_ds;
};

#endif /* __L4_EXEC_SERVER_EXC_IMG_H */

