#define __ASSEMBLY__
#include <l4/sys/compiler.h>

    .text
    .globl fpe_entry
fpe_entry:
    subl    $4, %esp
    pusha
    pushl   %esp
    call fpe_handler
    addl    $4, %esp
    popa
    addl    $8, %esp
    iret

    .globl div_entry
div_entry:
    subl    $4, %esp
    pusha
    pushl   %esp
    call div_handler
    addl    $4, %esp
    popa
    addl    $8, %esp
    iret
