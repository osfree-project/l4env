#define l4signal_ASSEMBLY__
#include <l4/sys/compiler.h>

/***********************************************************************/
/**
  * \file   libc_backends_l4env/lib/signal/except.S
  * \brief  Entry points for CPU exceptions
  *
  * \author Bjoern Doebel <doebel@os.inf.tu-dresden.de>
  */
/***********************************************************************/

    .text
    .globl l4signal_segv_entry
l4signal_segv_entry:
    subl $4, %esp
    pusha
    pushl %esp
    call l4signal_segv_handler
    addl $4, %esp
    popa
    addl $8, %esp
    iret

    .globl l4signal_fpe_entry
l4signal_fpe_entry:
    subl $4, %esp
    pusha
    pushl %esp
    call l4signal_fpe_handler
    addl $4, %esp
    popa
    addl $8, %esp
    iret

    .globl l4signal_illegal_entry
l4signal_illegal_entry:
    subl $4, %esp
    pusha
    pushl %esp
    call l4signal_illegal_handler
    addl $4, %esp
    popa
    addl $8, %esp
    iret

    .globl l4signal_stackf_entry
l4signal_stackf_entry:
    subl $4, %esp
    pusha
    pushl %esp
    call l4signal_illegal_handler
    addl $4, %esp
    popa
    addl $8, %esp
    iret
