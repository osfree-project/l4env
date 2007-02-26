/* $Id$ */
/*****************************************************************************
 * exec/include/l4/exec/exec.h                                               *
 *                                                                           *
 * Created:   08/18/2000                                                     *
 * Author(s): Frank Mehnert <fm3@os.inf.tu-dresden.de>                       *
 *                                                                           *
 * Common L4 environment                                                     *
 * L4 execution layer public interface.                                      *
 *****************************************************************************/

#ifndef _L4_EXEC_H
#define _L4_EXEC_H

/* flags for the type field */

/** section is readable */
#define L4_DSTYPE_READ		0x0001

/** section is writable */
#define L4_DSTYPE_WRITE		0x0002

/** section is executable */
#define L4_DSTYPE_EXECUTE	0x0004

/** section must be relocated, that is the start address of the section
 * has to be defined. This flag is set for dynamic sections. */
#define L4_DSTYPE_RELOCME	0x0008

/** section has to be linked */
#define L4_DSTYPE_LINKME	0x0010

/** section has to be paged by an external pager (old-style applications)
 * or by the region manager (new-style applications) */
#define L4_DSTYPE_PAGEME	0x0020

/** reserve space for section in vm_area of application */
#define L4_DSTYPE_RESERVEME	0x0040

/** if this flag is set the section is shared to the target address space,
 * else the section is direct used. */
#define L4_DSTYPE_SHARE		0x0080

/** first section of an ELF object */
#define L4_DSTYPE_OBJ_BEGIN	0x0100

/** last section of an ELF object */
#define L4_DSTYPE_OBJ_END	0x0200

/** there was an error while this section was dynamically linked,
 * e.g. a symbol reference could not be resolved */
#define L4_DSTYPE_ERRLINK	0x0400

/** this section is needed for startup */
#define L4_DSTYPE_STARTUP	0x0800

/** this section is owned by app */
#define L4_DSTYPE_APP_IS_OWNER	0x1000


/* flags for open */
#define L4EXEC_LOAD_SYMBOLS	0x0001	/* load symbols */
#define L4EXEC_LOAD_LINES	0x0002	/* load symbols */
#define L4EXEC_DIRECT_MAP	0x0004	/* program sections direct mapped */

#endif /* _L4_EXEC_H */

