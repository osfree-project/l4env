#ifndef __MACROS_H__
#define __MACROS_H__

#if defined ARCH_x86 | ARCH_arm
#define L4_CHAR_PTR		(char*)
#define L4_CONST_CHAR_PTR	(const char*)
#define L4_VOID_PTR		(void*)
#define L4_MB_MOD_PTR		(l4util_mb_mod_t*)
#define L4_MB_VBE_CTRL_PTR	(l4util_mb_vbe_ctrl_t*)
#define L4_MB_VBE_MODE_PTR	(l4util_mb_vbe_mode_t*)
#endif

#ifdef ARCH_amd64
#define L4_CHAR_PTR		(char*)(l4_addr_t)
#define L4_CONST_CHAR_PTR	(const char*)(l4_addr_t)
#define L4_VOID_PTR		(void*)(l4_addr_t)
#define L4_MB_MOD_PTR		(l4util_mb_mod_t*)(l4_addr_t)
#define L4_MB_VBE_CTRL_PTR	(l4util_mb_vbe_ctrl_t*)(l4_addr_t)
#define L4_MB_VBE_MODE_PTR	(l4util_mb_vbe_mode_t*)(l4_addr_t)
#endif

#endif  /* ! __MACROS_H__ */
