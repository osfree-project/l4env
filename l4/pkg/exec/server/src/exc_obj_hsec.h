/*!
 * \file	exc_obj_hsec.h
 * \brief	
 *
 * \date	2000
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the exec package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __EXC_OBJ_HSEC_H_
#define __EXC_OBJ_HSEC_H_

#include "exc_obj_psec.h"

/** section header descriptor */
typedef struct exc_obj_hsec_st {
    int			hs_type;
    int			hs_link;
    int			hs_size;
    l4_addr_t		vaddr;
    exc_obj_psec_t	*psec;
} exc_obj_hsec_t;

#endif /* __L4_EXEC_SERVER_EXC_OBJ_HSEC_H */

