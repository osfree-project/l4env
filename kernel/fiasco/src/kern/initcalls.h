#ifndef INITCALLS_H__
#define INITCALLS_H__

#define FIASCO_INIT		__attribute__ ((__section__ (".initcall.text")))
#define FIASCO_INITDATA		__attribute__ ((__section__ (".initcall.data")))

#define FIASCO_ASM_INIT		".section \".initcall.text\",\"ax\" \n"
#define FIASCO_ASM_INITDATA	".section \".initcall.data\",\"aw\" \n"
#define FIASCO_ASM_FINI		".previous \n"

#endif // INITCALLS_H__
