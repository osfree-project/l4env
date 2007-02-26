#ifndef __ARCH_AMD64_MACROS_H__
#define __ARCH_AMD64_MACROS_H__

/* We need this typecasts since addresses of the GRUB multiboot info
 * have always a size of 32 Bit. */

#define L4_CHAR_PTR(x)		(char*)(l4_addr_t)(x)
#define L4_CONST_CHAR_PTR(x)	(const char*)(l4_addr_t)(x)
#define L4_VOID_PTR(x)		(void*)(l4_addr_t)(x)
#define L4_MB_MOD_PTR(x)	(l4util_mb_mod_t*)(l4_addr_t)(x)

#endif  /* ! __ARCH_AMD64_MACROS_H__ */
