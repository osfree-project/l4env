#ifndef __L4_EXEC_SERVER_EXC_OBJ_HSEC_H
#define __L4_EXEC_SERVER_EXC_OBJ_HSEC_H

#include "exc_obj_psec.h"

/* section header descriptor */
typedef struct exc_obj_hsec_st {
    int			hs_type;
    int			hs_link;
    int			hs_size;
    l4_addr_t		vaddr;
    exc_obj_psec_t	*psec;
} exc_obj_hsec_t;

#endif /* __L4_EXEC_SERVER_EXC_OBJ_HSEC_H */

