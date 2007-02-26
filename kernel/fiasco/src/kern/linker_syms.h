#ifndef LINKER_SYMS_H
#define LINKER_SYMS_H

extern "C" {
  extern char _start, _ecode, _etext, _edata, _end;
  extern char _initcall_start, _initcall_end;	// Init Code & Data
  extern char _physmem_1;
  extern char _tcbs_1;
  extern char _iobitmap_1;
  extern char _unused1_1, _unused2_1, _unused3_1, _unused4_io_1;
  extern char _ipc_window0_1, _ipc_window1_1;
  extern char _service;
  extern char _smas_start_1, _smas_end_1;
  extern char _smas_version_1, _smas_area_1;
  extern char _task_sighandler_start, _task_sighandler_end;
  extern char _trampoline_page, _linuxstack;
  extern char _syscalls;
}

#endif // LINKER_SYMS_H
