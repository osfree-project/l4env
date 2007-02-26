#ifndef __GRUB_EMU_H
#define __GRUB_EMU_H

#include <stdio.h>
#include <string.h>

#define grub_sprintf	sprintf
#define grub_printf	printf
#define grub_memmove	memmove
#define grub_memset	memset
#define grub_memcmp	memcmp
#define grub_strcmp	strcmp
#define grub_strlen	strlen
#define grub_strcpy	strcpy
#define grub_putchar	putchar
#define grub_isspace	isspace

extern inline int getkey(int wait);
extern inline void *memmove(void * dest, const void * src, unsigned int n);

/* XXX function has to return a keycode if a key was pressed.
 * This allows us to abort the tftp loading process safely */
extern inline int
getkey(int wait)
{
  return 0;
}

#define CTRL_C		3
#define ASCII_CHAR(x)	((x) & 0xFF) 

/* memcpy when regions may overlap */
extern inline void
*memmove(void * dest, const void * src, unsigned int n)
{
  int d0, d1, d2;
  if (dest<src)
    __asm__ __volatile__(
	"cld\n\t"
	"rep\n\t"
	"movsb\n\t"
	:"=&c" (d0), "=&S" (d1), "=&D" (d2)
	:"0" (n), "1" (src), "2" (dest)
	:"memory");
  else
    __asm__ __volatile__(
	"std\n\t"
	"rep\n\t"
	"movsb\n\t"
	"cld\n\t"
	:"=&c" (d0), "=&S" (d1), "=&D" (d2)
	:"0" (n),
	 "1" (n-1+(const char *)src),
	 "2" (n-1+(char *)dest)
	:"memory");
  return dest;
}

#endif /* __GRUB_EMU_H */

