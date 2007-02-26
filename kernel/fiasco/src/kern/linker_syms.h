#ifndef LINKER_SYMS_H
#define LINKER_SYMS_H

extern "C" {
  extern char _start, _ecode, _etext, _edata, _end;
  extern char _initcall_start, _initcall_end;	// Init Code & Data
  extern char _tcbs_1, _physmem_1, _syscalls;	// Linkerscript
  extern char _task_sighandler_start, _task_sighandler_end, _trampoline_page;
  extern char _utcb_address_page;
}

#endif // LINKER_SYMS_H
