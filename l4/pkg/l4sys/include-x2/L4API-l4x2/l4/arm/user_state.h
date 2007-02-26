/*********************************************************************
 *                
 * Copyright (C) 2002, 2003   University of New South Wales
 * Copyright (C) 2003,	      National ICT Australia
 *                
 * File path:     l4/arm/user_state.h
 * Description:   ARM specific user_state structure definition
 *                
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *                
 * $Id$
 *                
 ********************************************************************/
#ifndef __L4__ARM__USER_STATE_H__
#define __L4__ARM__USER_STATE_H__

typedef struct {
    L4_Word_t klr;    /* -0 - */
    L4_Word_t pc;     /*  4   */
    L4_Word_t cpsr;   /*  8   */
    L4_Word_t  r0;     /*  12  */
    L4_Word_t  r1;     /*  16  */
    L4_Word_t  r2;     /* -20- */
    L4_Word_t  r3;     /*  24  */
    L4_Word_t  r4;     /*  28  */
    L4_Word_t  r5;     /*  32  */
    L4_Word_t  r6;     /*  36  */
    L4_Word_t  r7;     /* -40- */
    L4_Word_t  r8;     /*  44  */
    L4_Word_t  r9;     /*  48  */
    L4_Word_t  r10;    /*  52  */
    L4_Word_t  r11;    /*  56  */
    L4_Word_t  r12;    /* -60- */
    L4_Word_t  sp;     /*  64  */
    L4_Word_t  lr;     /*  68  */
} armGPRegs_t;

typedef struct {
    armGPRegs_t gpRegs;
} UserState_t;

#endif /* !__L4__ARM__USER_STATE_H__ */
