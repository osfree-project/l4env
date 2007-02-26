#ifndef IDT_INIT
#define IDT_INIT

#ifdef ASSEMBLER

#define GATE_INITTAB_BEGIN(name)	\
	.section ".initcall.data"	;\
	.globl	name			;\
name:					;\
	.text

#define	GATE_ENTRY(n,name,type)	\
	.section ".initcall.data"	;\
	.long	name			;\
	.word	n			;\
	.word	type			;\
	.text

#define GATE_INITTAB_END		\
	.section ".initcall.data"	;\
	.long	0			;\
	.text

#define SEL_PL_U	0x03
#define SEL_PL_K	0x00

#define ACC_TASK_GATE	0x05
#define ACC_INTR_GATE	0x0e
#define ACC_TRAP_GATE	0x0f
#define ACC_PL_U	0x60
#define ACC_PL_K	0x00

#else // !ASSEMBLER

#include "l4_types.h"

typedef struct
{
  Unsigned32  entry;
  Unsigned16  vector __attribute__((packed));
  Unsigned16  type   __attribute__((packed));
} Idt_init_entry;

extern Idt_init_entry idt_init_table[];

#endif

#endif

