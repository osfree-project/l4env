/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4sys/include/l4/sys/xadaption.h
 * \brief  Macros to adapt L4 V2 syscall bindings to L4 X.0
 *
 * \date   02/23/2002
 * \author Volkmar Uhlig <ulig@ira.uka.de>
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/
#ifndef __L4_XADAPTION_H__
#define __L4_XADAPTION_H__

/*****************************************************************************
 *** 64-bit Id -> 32-bit Id
 *****************************************************************************/

/* EDI/ESI -> ESI */
#define	ToId32_EdiEsi \
  "shll   $7,%%edi            \n\t"   /* chief -> bits 24-31 */              \
  "andl   $0x01feffff,%%esi   \n\t"   /* clear unused task/thread bits */    \
  "rcll   $16,%%esi	      \n\t"   /* task -> bits 0-7 */                 \
  "andl   $0xff000000,%%edi   \n\t"   /* clear non-chief bits in EDI */      \
  "rorl   $16,%%esi           \n\t"   /* thread/ver0->0-15, task->16-23 */   \
  "addl   %%edi,%%esi         \n\t"   /* set chief in ESI */

/* EBP/EBX -> EBX */
#define	ToId32_EbpEbx \
  "shll   $7,%%ebp            \n\t"  \
  "andl   $0x01feffff,%%ebx   \n\t"  \
  "rcll   $16,%%ebx           \n\t"  \
  "andl   $0xff000000,%%ebp   \n\t"  \
  "rorl   $16,%%ebx           \n\t"  \
  "addl   %%ebp,%%ebx         \n\t" 

/* EAX (64-bit id low) -> EAX, used in task_new */ 
#define	ToId32_Eax \
  "cmpl   $0x100,%%eax        \n\t"   /* only adapt if task is specified */  \
  "jb     1f                  \n\t"   /* not if mcp is specified */          \
  "andl   $0x01feffff,%%eax   \n\t"   /* clear unused task/thread bits */    \
  "rcll   $16,%%eax           \n\t"   /* task -> bits 0-7 */                 \
  "rorl   $16,%%eax           \n\t"   /* thread/ver0->0-15, task->16-23 */   \
  "1:                         \n\t"

/*****************************************************************************
 *** 32-bit Id -> 64-bit Id
 *****************************************************************************/

/* ESI -> EDI/ESI */ 
#define FromId32_Esi \
  "movl	  %%esi,%%edi         \n\t"                                          \
  "andl	  $0x00ffffff,%%esi   \n\t"   /* clear ver1 / task bits 8-10 */      \
  "roll	  $16,%%esi           \n\t"   /* thread / ver0 -> bits 16-31 */      \
  "andl	  $0xff000000,%%edi   \n\t"   /* clear site bits in EDI */           \
  "rcrl	  $16,%%esi           \n\t"   /* thread/ver0->0-16, task->17-27 */   \
  "shrl	  $7,%%edi            \n\t"   /* chief -> 17-27 in EDI */

/* EBX -> EBP/EBX */ 
#define FromId32_Ebx \
  "movl	  %%ebx,%%ebp         \n\t"  \
  "andl	  $0x00ffffff,%%ebx   \n\t"  \
  "roll	  $16,%%ebx           \n\t"  \
  "andl	  $0xff000000,%%ebp   \n\t"  \
  "rcrl	  $16,%%ebx           \n\t"  \
  "shrl	  $7,%%ebp            \n\t"

/*****************************************************************************
 *** Set message dword 2
 *****************************************************************************/

#define FixLongIn \
  "testl  $0xfffffffc,%%eax   \n\t"   /* only set dw2 for long IPC */        \
  "jz     1f                  \n\t"                                          \
  "movl   %%eax,%%edi         \n\t"                                          \
  "andl   $0xfffffffc,%%edi   \n\t"                                          \
  "movl   20(%%edi),%%edi     \n\t"   /* load dw2 to EDI */                  \
  "1:                         \n\t"                                          \
  "pushl  %%ebp               \n\t"   /* save receive message dope */

#define FixLongOut \
  "popl   %%ebp               \n\t"   /* load receive message dope */        \
  "testb  $0xf0,%%al          \n\t"   /* only adapt if no error */           \
  "jnz    2f                  \n\t"                                          \
  "testl  $0x02,%%ebp         \n\t"                                          \
  "jnz    1f                  \n\t"   /* rmap so discard the third dw */     \
  "andl   $0xfffffffc,%%ebp   \n\t"                                          \
  "jz     1f                  \n\t"   /* short receive (discard third dw */  \
  "movl   4(%%ebp),%%ecx      \n\t"                                          \
  "shrl   $13,%%ecx           \n\t"                                          \
  "cmp    $2,%%ecx            \n\t"                                          \
  "jle    1f                  \n\t"                                          \
  "movl   %%edi,20(%%ebp)     \n\t"   /* move dw2 to message buffer */       \
  "jmp    2f                  \n\t"                                          \
  "1:                         \n\t"                                          \
  "andl   $0x00001fff,%%eax   \n\t"   /* set rcv dwords to 2 */              \
  "orl    $0x00004000,%%eax   \n\t"                                          \
  "2:                         \n\t" 
                  
#define FixLongStackIn \
  "pushl  %%ebp               \n\t"   /* save receive message dope */

#define FixLongStackOut \
  "addl   $4,%%esp            \n\t"   /* remove receive dope from stack */

/*****************************************************************************
 *** prototypes
 *****************************************************************************/

L4_INLINE l4_umword_t
l4sys_to_id32(l4_threadid_t id);

L4_INLINE l4_threadid_t
l4sys_from_id32(l4_umword_t id);

/*****************************************************************************
 *** implementation
 *****************************************************************************/

L4_INLINE l4_umword_t
l4sys_to_id32(l4_threadid_t id)
{
  l4_umword_t tmp,dummy;
  
  __asm__
    (
     ToId32_EdiEsi
     :
     "=S" (tmp),
     "=D" (dummy)
     :
     "S"  (id.lh.low),
     "D"  (id.lh.high)
     );

  return tmp;
}

L4_INLINE l4_threadid_t
l4sys_from_id32(l4_umword_t id)
{
  l4_threadid_t id_long;
  
  __asm__
    (
     FromId32_Esi
     :
     "=S" (id_long.lh.low),
     "=D" (id_long.lh.high)
     :
     "S"  (id)
     );

  return id_long;
}

#endif /* __L4_XADAPTION_H__ */
