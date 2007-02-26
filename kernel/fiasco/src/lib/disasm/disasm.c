
#include <malloc.h>
#include <stdarg.h>
#include <string.h>
#include <flux/x86/paging.h>
#include "dis-asm.h"

/* local variables */
static char *out_buf;
static int out_len;
static int use_syms;
static unsigned dis_task;
static unsigned dis_va2;
static unsigned dis_pa1;
static unsigned dis_pa2;
static disassemble_info dis_info;

/* extern function to print address or symbol at address */
extern const char*
disasm_get_symbol_at_address(unsigned address, unsigned task);

/* prototype of the function we export */
extern unsigned int
disasm_bytes(char *buffer, unsigned len, unsigned va, unsigned pa1,
	     unsigned pa2, unsigned pa_size, unsigned task, 
	     int show_symbols, int show_intel_syntax);


/* read <length> bytes starting at memaddr */
static int
my_read_memory(bfd_vma memaddr, bfd_byte *myaddr, unsigned int length,
	       struct disassemble_info *info __attribute__ ((unused)))
{
  unsigned opb  = info->octets_per_byte;
  unsigned end_addr_offset = length / opb;
  unsigned max_addr_offset = info->buffer_length / opb;
  unsigned pa   = (memaddr < dis_va2) ? dis_pa1 : dis_pa2;
  unsigned size = PAGE_SIZE - (length & PAGE_MASK);

  if (   memaddr < info->buffer_vma
      || memaddr - info->buffer_vma + end_addr_offset > max_addr_offset)
    return -1;

  if (size > length)
    size = length;

  if (size > 0)
    memcpy(myaddr, (void*)(pa + (memaddr & PAGE_MASK)), size);

  if ((length -= size))
    memcpy(myaddr+size, (void*)dis_pa2, length);

  return 0;
}

/* XXX return byte without range check */
unsigned char
my_get_data(bfd_vma memaddr)
{
  if (memaddr < dis_va2)
    return *(unsigned char*)(dis_pa1 + (memaddr & PAGE_MASK));

  return *(unsigned char*)(dis_pa2 + (memaddr & PAGE_MASK));
}

/* print address (using symbols if possible) */
static void
my_print_address(bfd_vma memaddr, struct disassemble_info *info)
{
  const char *symbol;

  if (use_syms && (symbol = disasm_get_symbol_at_address(memaddr, dis_task)))
    {
      unsigned i;
      char buf[48];
      const char *s;
      
      buf[0] = ' ';
      buf[1] = '<';
      for (i=2, s=symbol; i<(sizeof(buf)-3); )
	{
	  if (*s=='\0' || *s=='\n')
	    break;
	  buf[i++] = *s++;
	  if (*(s-1)=='(')
	    while ((*s != ')') && (*s != '\n') && (*s != '\0'))
	      s++;
	}
      buf[i++] = '>';
      buf[i++] = '\0';

      (*info->fprintf_func)(info->stream, "%s", buf);
    }
}

static int
my_printf(void* stream __attribute__ ((unused)), const char *format, ...)
{
  if (out_len)
    {
      int len;
      va_list list;
  
      va_start(list, format);
      len = vsnprintf(out_buf, out_len, format, list);
      out_buf += len;
      out_len -= len;
      va_end(list);
    }
  
  return 0;
}

static void
my_putchar(int c)
{
  if (out_len)
    {
      out_len--;
      *out_buf++ = c;
    }
}

/* check for special L4 int3-opcodes */
static int
special_l4_ops(bfd_vma memaddr)
{
  int len, bytes, i;
  const unsigned char *op;
  bfd_vma str, s;
  
  switch (my_get_data(memaddr))
    {
    case 0xeb:
      op  = "enter_kdebug";
      len = my_get_data(memaddr+1);
      str = memaddr+2;
      bytes = 3+len;
      goto printstr;
    case 0x90:
      if (my_get_data(memaddr+1) != 0xeb)
	break;
      op  = "kd_display";
      len = my_get_data(memaddr+2);
      str = memaddr+3;
      bytes = 4+len;
    printstr:
      /* do a qick test if it is really a int3-str function by
       * analyzing the bytes we shall display. */
      for (i=len, s=str; i--; )
	if (my_get_data(s++) > 126)
	  return 0;
      /* test well done */
      my_printf(0, "<%s (\"", op);
      if ((out_len > 2) && (len > 0))
	{
	  out_len -= 3;
     	  if (out_len > len)
	    out_len = len;
	  /* do not use my_printf here because the string
	   * can contain special characters (e.g. tabs) which 
	   * we do not want to display */
	  while (out_len)
	    {
	      unsigned char c = my_get_data(str++);
	      my_putchar((c<' ') ? ' ' : c);
	    }
	  out_len += 3;
	}
      my_printf(0, "\")>");
      return bytes;
    case 0x3c:
      op = NULL;
      switch (my_get_data(memaddr+1))
	{
	case 0: op = "outchar (%al)";  break;
	case 2: op = "outstring (*%eax)"; break;
	case 5: op = "outhex32 (%eax)"; break;
	case 6: op = "outhex20 (%eax)"; break;
	case 7: op = "outhex16 (%eax)"; break;
	case 8: op = "outhex12 (%eax)"; break;
	case 9: op = "outhex8 (%al)"; break;
	case 11: op = "outdec (%eax)"; break;
	case 13: op = "%al = inchar ()"; break;
	case 24: op = "fiasco_start_profile()"; break;
	case 25: op = "fiasco_stop_and_dump()"; break;
	case 26: op = "fiasco_stop_profile()"; break;
	case 30: op = "fiasco_register (%eax, %ecx)"; break;
	}
      if (op)
	my_printf(0, "<%s>", op);
      else if (my_get_data(memaddr+1) >= ' ')
	my_printf(0, "<ko ('%c')>", my_get_data(memaddr+1));
      else break;
      return 3;
    }

  return 0;
}

/* WARNING: This function is not reentrant because it accesses some
 * global static variables (out_buf, out_len, dis_task, ...) */
unsigned int
disasm_bytes(char *buffer, unsigned len, unsigned va, unsigned pa1,
	     unsigned pa2, unsigned pa_size, unsigned task, 
	     int show_symbols, int show_intel_syntax)
{
  use_syms = show_symbols;
  out_buf  = buffer;
  out_len  = len;
  dis_task = task;
  dis_pa1  = pa1 & ~PAGE_MASK;
  dis_pa2  = pa2 & ~PAGE_MASK;
  dis_va2  = round_page(va+1);

  /* terminate string */
  if (out_len)
    out_buf[--out_len] = '\0';

  /* sanity check */
  if (pa_size == 0)
    return 0;

  /* test for special L4 opcodes */
  if (   (my_get_data(va) == 0xcc)
      && ((len = special_l4_ops(va+1))))
    return len;

  /* one step back for special L4 opcodes */
  if (   (va & 0xfff) /* don't cross page backwards */
      && (my_get_data(va-1) == 0xcc)
      && ((len = special_l4_ops(va))))
    return len-1;
  
  INIT_DISASSEMBLE_INFO(dis_info, NULL, my_printf);
  
  dis_info.print_address_func = my_print_address;
  dis_info.read_memory_func = my_read_memory;
  dis_info.buffer = (bfd_byte*)va;
  dis_info.buffer_length = pa_size;
  dis_info.buffer_vma = va;

  return (show_intel_syntax) 
    ? print_insn_i386_intel(va, &dis_info)
    : print_insn_i386_att(va, &dis_info);
}

