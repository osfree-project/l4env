#ifndef __EXEC___CONFIG_H
#define __EXEC___CONFIG_H

#define HEAP_SIZE   (4*1024*1024)  /* heap for malloc (in particular for new) */

#define MAX_PSECS	1024	/* max # of program sections exec can handle */

#define EXC_MAXPSECT	6	/* max # of program sections per ELF object */
#define EXC_MAXHSECT	64	/* max # of header sections per ELF object */
#define EXC_MAXFNAME	4096	/* max # of chars in elf_obj.fname */
#define EXC_MAXDEP	16	/* max # of dependant libs per ELF object */

#define EXC_MAXBIN	32	/* max # of binary objects */ 
#define EXC_MAXOBJ	256	/* max # of ELF objects */
#define EXC_MAXLIB	32	/* max # of libraries per binary object */

#endif /* __EXEC___CONFIG_H */

