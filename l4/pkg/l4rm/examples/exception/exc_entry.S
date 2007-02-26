/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4rm/examples/exception/exc_entry.S
 * \brief  Exception entry point
 *
 * \date   08/16/2004
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#define __ASSEMBLY__
#include <l4/sys/compiler.h>

        .text
        .globl pf_entry
pf_entry:
        subl    $4, %esp        /* save page-fault address */
        pusha
        pushl   %esp
        call    pf_handler
        addl    $4, %esp
        popa
        addl    $8, %esp        /* remove page-fault address / error code */
        iret
