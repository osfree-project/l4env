extern void __attribute__((noreturn)) PREFIX(pong_short_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_short_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_short_cold_thread)(void);
extern void __attribute__((noreturn)) PREFIX(pong_short_to_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_short_to_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_short_to_cold_thread)(void);
extern void __attribute__((noreturn)) PREFIX(pong_short_dc_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_short_dc_thread)(void);
extern void __attribute__((noreturn)) PREFIX(pong_short_ndc_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_short_ndc_thread)(void);
extern void __attribute__((noreturn)) PREFIX(pong_short_c_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_short_c_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_short_c_cold_thread)(void);
extern void __attribute__((noreturn)) PREFIX(pong_short_asm_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_short_asm_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_short_asm_cold_thread)(void);
extern void __attribute__((noreturn)) PREFIX(pong_long_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_long_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_long_cold_thread)(void);
extern void __attribute__((noreturn)) PREFIX(pong_indirect_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_indirect_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_indirect_cold_thread)(void);
extern void __attribute__((noreturn)) PREFIX(pong_fpage_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_fpage_thread)(void);
extern void __attribute__((noreturn)) PREFIX(pong_long_fpage_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_long_fpage_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_long_fpage_cold_thread)(void);
extern void __attribute__((noreturn)) PREFIX(pong_pagefault_thread)(void);
extern void __attribute__((noreturn)) PREFIX(ping_pagefault_thread)(void);
extern void __attribute__((noreturn)) PREFIX(syscall_id_nearest_thread)(void);
extern void __attribute__((noreturn)) PREFIX(syscall_task_new_thread)(void);
