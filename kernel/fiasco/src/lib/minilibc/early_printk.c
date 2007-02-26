#ifndef EARLY_PRINTK_H__
#define EARLY_PRINTK_H__

#include <asm/io.h>
#include <string.h>

#define VGABASE		((char *)0xc0000000000b8000)
#define VGALINES	25
#define VGACOLS		80

static int current_ypos = VGALINES, current_xpos = 0;


void
early_printk (const char *str, unsigned len)
{
  char c;
  int  i, k, j;
  
  while ((*str != 0) && (len-- > 0)) {
    c= *str++;
    if (current_ypos >= VGALINES) {
      /* scroll 1 line up */
      memmove(VGABASE, (VGABASE+2*VGACOLS), 2*((VGALINES-1)*VGACOLS));
      /*
      
      for (k = 1, j = 0; k < VGALINES; k++, j++) {
	for (i = 0; i < VGACOLS; i++) {
	  writew(readw(VGABASE + 2*(VGACOLS*k + i)),
		 VGABASE + 2*(VGACOLS*j + i));
	}
      }
      */
      for (i = 0; i < VGACOLS; i++) {
	writew(0x0720, VGABASE + 2*(VGACOLS*(VGALINES-1) + i));
      }
      current_ypos = VGALINES-1;
    }
    if (c == '\n') {
      current_xpos = 0;
      current_ypos++;
    } else if (c != '\r')  {
      writew(0x0f00 | (unsigned short)c,
	     VGABASE + 2*(VGACOLS*current_ypos + current_xpos++));
      if (current_xpos >= VGACOLS) {
	current_xpos = 0;
	current_ypos++;
      }
    }
  }
}

#endif /* EARLY_PRINTK_H__ */
