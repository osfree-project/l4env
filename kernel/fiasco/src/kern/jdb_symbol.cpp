INTERFACE:

class Jdb_symbol
{
private:
  typedef struct
    {
      const char *str;
      unsigned beg;
      unsigned end;
    } task_symbol_t;
  
  static task_symbol_t task_symbols[2048];
};


IMPLEMENTATION:

#include <cstdio>
#include <cstring>
#include "kmem.h"

Jdb_symbol::task_symbol_t Jdb_symbol::task_symbols[2048];

// read address from current position and return its value
static inline
unsigned
Jdb_symbol::string_to_addr(char *symstr)
{
  unsigned address = 0;

  for (int i=0; i<= 7; i++)
    {
      switch (unsigned c = *symstr++)
	{
	case 'a' ... 'f':
	  c -= 'a' - '9' - 1;
	case '0' ... '9':
	  c -= '0';
	  address <<= 4;
	  address += c;
	}
    }

  return address;
}

// optimize symbol table to speed up address-to-symbol search
// replace stringified address (8 characters) by 2 dwords of
// 32 bit: the symbol value and the absolute address of the
// next symbol (or 0 if end of list)
static
bool
Jdb_symbol::set_symbols(unsigned task, unsigned addr)
{
  if (addr == 0)
    {
      // disable symbols for specific tasks
      task_symbols[task].str = 0;
      return false;
    }

  char *symstr = (char*)Kmem::phys_to_virt(addr);
  task_symbols[task].str = symstr;
  
  for (int line=0; ; line++)
    {
      char *sym = symstr;
      
      while (((*symstr) != '\0') && (*symstr != '\n'))
	symstr++;
      
      if (symstr-sym < 11)
	{
	  if (line == 0)
	    {
	      printf("Invalid symbol table of task #%x at line %d "
	             "-- disabling symbols\n", task, line);
	      task_symbols[task].str = 0;
	      return false;
	    }

	  ((unsigned*)sym)[-1] = 0;  // terminate list
	  ((unsigned*)sym)[ 0] = 0;  // terminate list
	  *symstr = '\0';            // terminate string
	  return true;
	}
      
      // write binary address
      unsigned address = string_to_addr(sym);
      *((unsigned*)sym)++ = address;
      
      if (!*symstr || !*(symstr+1))
	{
	  *((unsigned*)sym) = 0;  // terminate list
	  *symstr = '\0';  // terminate string
	  return true;
	}
      
      // terminate current symbol (overwrite '\n')
      *symstr = '\0';

      // link address
      *((unsigned*)sym)++ = (unsigned)(++symstr);
    }
}

// determine if symbol is included in kernel symbols 
// by checking the address
static inline
bool
Jdb_symbol::addr_in_kernel(unsigned addr)
{
  return (   (task_symbols[0].str != 0)
          && (task_symbols[0].beg <= addr));
}

// search symbol in task's symbols, return pointer to symbol name
static
const char*
Jdb_symbol::match_symbol(const char *symbol, bool search_instr, unsigned task)
{
  const char *sym, *symnext;

  // walk through list of symbols
  for (sym=task_symbols[task].str; sym; sym=symnext)
    {
      symnext = *((const char**)sym+1);
      
      // search symbol
      if (  ( search_instr && ( strstr(sym+11, symbol) == sym+11))
	  ||(!search_instr && (!strcmp(sym+11, symbol))))
	return sym;
    }

  return 0;
}

// Search a symbol in symbols of a specific task. If the symbol
// was not found, search in kernel symbols too (if exists)
PUBLIC static
unsigned 
Jdb_symbol::match_symbol_to_address(const char *symbol, bool search_instr,
				    unsigned task)
{
  const char *sym;
  
  if (task >= 2048)
    return 0;

  for (;;)
    {
      if (    (task_symbols[task].str == 0)
	  || !(sym = match_symbol(symbol, search_instr, task)))
	{
	  // no symbols for task or symbol not found in symbols
	  if (task != 0)
	    {
	      task = 0; // not yet searched in kernel symbols, do it now
	      continue;
	    }
	  else
	    return 0; // already searched in kernel symbols so we fail
	}

      return *(unsigned*)sym;
    }
}

// try to search a possible symbol completion
PUBLIC static
bool
Jdb_symbol::complete_symbol(char *symbol, unsigned size, unsigned task)
{
  unsigned equal_len = 0xffffffff, symbol_len = strlen(symbol);
  const char *sym, *symnext, *equal_sym = 0;

  if (symbol_len >= --size)
    return false;

repeat:
  if ((task <= 2048) && (task_symbols[task].str != 0))
    {
      // walk through list of symbols
      for (sym=task_symbols[task].str; sym; sym=symnext)
	{
	  symnext = *((const char**)sym+1);
	  sym += 11;
      
	  // search symbol
	  if (strstr(sym, symbol) == sym)
	    {
	      if (!equal_sym)
		{
		  equal_len = strlen(sym);
		  equal_sym = sym;
		}
	      else
		{
		  unsigned c = 0;
		  const char *s1 = sym, *s2 = equal_sym;
	  
		  while (*s1++ == *s2++)
		    c++;
	      
		  if (c < equal_len)
		    {
		      equal_len = c;
		      equal_sym = sym;
		    }
		}
	    }
	}
    }

  if (equal_sym)
    {
      if (equal_len > symbol_len)
	{
	  // unique completion found, append it to symbol
	  if (equal_len > size)
	    equal_len = size;
      
	  strncpy(symbol, equal_sym, equal_len);
	  symbol[equal_len] = '\0';
	  return true;
	}
    }
  else
    {
      // no completion found or no symbols
      if (task != 0)
	{
	  // repeat search with kernel symbols
	  task = 0;
	  goto repeat;
	}

      int c;
      if (   (symbol_len < 1)
	  || (((c = symbol[symbol_len-1]) != '?') && (c != '*')))
	{
	  symbol[symbol_len++] = '?';
	  symbol[symbol_len]   = '\0';
	  return true;
	}
    }

  return false;
}

// search symbol name that matches for a specific address
PUBLIC static
const char*
Jdb_symbol::match_address_to_symbol(unsigned address, unsigned task)
{
  if (addr_in_kernel(address))
    task = 0;

  if (task>=2048)
    return 0;

  if (   (task_symbols[task].str != 0)
      && (task_symbols[task].beg <= address)
      && (task_symbols[task].end >= address))
    {
      const char *sym;
      for (sym = task_symbols[task].str; sym; sym = *((const char**)sym+1))
	{
	  if ((*(unsigned*)sym == address)
	      && (memcmp((void*)(sym+11), "Letext", 6))		// ignore
	      && (memcmp((void*)(sym+11), "patch_log_", 10))	// ignore
	     )
	    return sym+11;
	}
    }

  return 0;
}

// search last symbol before eip
PUBLIC static
bool
Jdb_symbol::match_eip_to_symbol(unsigned eip, unsigned task,
				char *t_symbol, int s_symbol)
{
  if (addr_in_kernel(eip))
    task = 0;

  if (task>=2048)
    return false;

  if (   (task_symbols[task].str != 0)
      && (task_symbols[task].beg <= eip)
      && (task_symbols[task].end >= eip))
    {
      const char *sym, *max_sym = 0;
      unsigned max_addr = 0;
      
      for (sym = task_symbols[task].str; sym; sym = *((const char**)sym+1))
	{
	  unsigned addr = *(unsigned*)sym;
	  if (   (addr > max_addr) && (addr <= eip)
	      && (memcmp((void*)(sym+11), "Letext", 6))		// ignore
	      && (memcmp((void*)(sym+11), "patch_log_", 10))	// ignore
	     )
	    {
	      max_addr = addr;
	      max_sym  = sym;
	    }
	}
      
      if (max_sym)
	{
	  const char *t = max_sym + 11;
	  
	  t_symbol[--s_symbol] = '\0';
	  while ((*t != '\n') && (*t != '\0') && (s_symbol--))
	    {
	      *t_symbol++ = *t++;

	      // print functions with arguments as ()
	      if (*(t-1) == '(')
		while (*t != ')' && (*t != '\n') && (*t != '\0')) t++;
	    }
	  
	  // terminate string
	  if (s_symbol)
	    *t_symbol = '\0';

	  return true;
	}
    }

  return false;
}

// register symbols for a specific task (called by user mode application)
PUBLIC static
void
Jdb_symbol::register_symbols(unsigned addr, unsigned task)
{
  // sanity check
  if (task >= 2048)
    {
      printf("register_symbols: task value %d out of range (0-2047)\n", task);
      return;
    }
  
  if (!set_symbols(task, addr))
    return;

  if (task == 0)
    {
      // kernel symbols are special
      unsigned sym_start, sym_end;
      
      if (  !(sym_start = match_symbol_to_address("_start", false, 0))
    	  ||!(sym_end   = match_symbol_to_address("_end", false, 0)))
	{
	  printf("KERNEL: Missing \"_start\" or \"_end\" in kernel symbols "
	      "-- disabling kernel symbols\n");
	  task_symbols[0].str = 0;
	  return;
	}

      if (sym_end != (unsigned)&_end)
	{
	  printf("KERNEL: Fiasco symbol table does not match kernel "
 	      "-- disabling kernel symbols\n");
     	  task_symbols[0].str = 0;
	  return;
	}

      task_symbols[0].beg = sym_start;
      task_symbols[0].end = sym_end;
    }
  else
    {
      unsigned min_addr = 0xffffffff, max_addr = 0;
      const char *sym;

      for (sym = task_symbols[task].str; sym; sym = *((const char**)sym+1))
	{
	  unsigned addr = *(unsigned*)sym;
	  if (addr < min_addr)
	    min_addr = addr;
	  if (addr > max_addr)
	    max_addr = addr;
	}
      
      task_symbols[task].beg = min_addr;
      task_symbols[task].end = max_addr;
    }
}

