#ifndef MY_PTRACE_H
#define MY_PTRACE_H

#define EBX 0
#define ECX 1
#define EDX 2
#define ESI 3
#define EDI 4
#define EBP 5
#define EAX 6
/* no ds, es , fs, gs here */
#define ORIG_EAX 7
#define EIP 8
#define CS  9
#define EFL 10
#define UESP 11
/* no ss here */
#define EXCEPTION_NUMBER 12
#define ERROR_CODE 13


#ifndef __ASSEMBLY__

/* this struct defines the way the registers are stored on the 
   stack during a system call. */

typedef struct {
	long ebx;
	long ecx;
	long edx;
	long esi;
	long edi;
	long ebp;
	long eax;
	long orig_eax;
	long eip;
	int  __xcs;
	long eflags;
	long esp;
  /* some extentions */
  long exception_number;
  long error_code;
} pt_regs_t;


#endif /* __ASSEMBLY__ */
#endif /* MY_PTRACE_H */
