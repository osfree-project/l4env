INTERFACE:

#include "l4_types.h"

class Jdb_symbol
{
private:
  typedef struct
    {
      const char *str;
      Address    beg;
      Address    end;
    } Task_symbol;
  
  static Task_symbol task_symbols[L4_uid::MAX_TASKS];
};


IMPLEMENTATION:

#include <cstdio>
#include <cstring>
#include "kmem.h"

Jdb_symbol::Task_symbol Jdb_symbol::task_symbols[L4_uid::MAX_TASKS];

// read address from current position and return its value
static inline
Address
Jdb_symbol::string_to_addr(char *symstr)
{
  Address addr = 0;

  for (int i=0; i<= 7; i++)
    {
      switch (Address c = *symstr++)
	{
	case 'a': case 'b': case 'c':
	case 'd': case 'e': case 'f':
	  c -= 'a' - '9' - 1;
	case '0': case '1': case '2':
	case '3': case '4': case '5':
	case '6': case '7': case '8':
	case '9':
	  c -= '0';
	  addr <<= 4;
	  addr += c;
	}
    }

  return addr;
}

// optimize symbol table to speed up address-to-symbol search
// replace stringified address (8 characters) by 2 dwords of
// 32 bit: the symbol value and the absolute address of the
// next symbol (or 0 if end of list)
static
bool
Jdb_symbol::set_symbols(Task_num task, Address addr)
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

	  ((Address*)sym)[-1] = 0;  // terminate list
	  ((Address*)sym)[ 0] = 0;  // terminate list
	  *symstr = '\0';            // terminate string
	  return true;
	}
      
      // write binary address
      Address addr = string_to_addr(sym);
      *((Address*)sym)++ = addr;
      
      if (!*symstr || !*(symstr+1))
	{
	  *((Address*)sym) = 0;  // terminate list
	  *symstr = '\0';  // terminate string
	  return true;
	}
      
      // terminate current symbol (overwrite '\n')
      *symstr = '\0';

      // link address
      *((Address*)sym)++ = (Address)(++symstr);
    }
}

// search symbol in task's symbols, return pointer to symbol name
static
const char*
Jdb_symbol::match_symbol(const char *symbol, bool search_instr, Task_num task)
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
Address 
Jdb_symbol::match_symbol_to_addr(const char *symbol, bool search_instr,
				 Task_num task)
{
  const char *sym;

  if (task >= L4_uid::MAX_TASKS)
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

      return *(Address*)sym;
    }
}

// try to search a possible symbol completion
PUBLIC static
bool
Jdb_symbol::complete_symbol(char *symbol, unsigned size, Task_num task)
{
  unsigned equal_len = 0xffffffff, symbol_len = strlen(symbol);
  const char *sym, *symnext, *equal_sym = 0;

  if (symbol_len >= --size)
    return false;

repeat:
  if ((task <= L4_uid::MAX_TASKS) && (task_symbols[task].str != 0))
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
Jdb_symbol::match_addr_to_symbol(Address addr, Task_num task)
{
  if (task>=L4_uid::MAX_TASKS)
    return 0;

  if (   (task_symbols[task].str != 0)
      && (task_symbols[task].beg <= addr)
      && (task_symbols[task].end >= addr))
    {
      const char *sym;
      for (sym = task_symbols[task].str; sym; sym = *((const char**)sym+1))
	{
	  if ((*(Address*)sym == addr)
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
Jdb_symbol::match_eip_to_symbol(Address eip, Task_num task, 
				char *t_symbol, int s_symbol)
{
  if (task>=L4_uid::MAX_TASKS)
    return false;

  if (   (task_symbols[task].str != 0)
      && (task_symbols[task].beg <= eip)
      && (task_symbols[task].end >= eip))
    {
      const char *sym, *max_sym = 0;
      Address max_addr = 0;
      
      for (sym = task_symbols[task].str; sym; sym = *((const char**)sym+1))
	{
	  Address addr = *(Address*)sym;
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
Jdb_symbol::register_symbols(Address addr, Task_num task)
{
  // sanity check
  if (task >= L4_uid::MAX_TASKS)
    {
      printf("register_symbols: task value %d out of range (0-%d)\n", 
	  task, L4_uid::MAX_TASKS-1);
      return;
    }
  
  if (!set_symbols(task, addr))
    return;

  if (task == 0)
    {
      // kernel symbols are special
      Address sym_start, sym_end;
      
      if (  !(sym_start = match_symbol_to_addr("_start", false, 0))
    	  ||!(sym_end   = match_symbol_to_addr("_end", false, 0)))
	{
	  printf("KERNEL: Missing \"_start\" or \"_end\" in kernel symbols "
	      "-- disabling kernel symbols\n");
	  task_symbols[0].str = 0;
	  return;
	}

      if (sym_end != (Address)&_end)
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
      Address min_addr = 0xffffffff, max_addr = 0;
      const char *sym;

      for (sym = task_symbols[task].str; sym; sym = *((const char**)sym+1))
	{
	  Address addr = *(Address*)sym;
	  if (addr < min_addr)
	    min_addr = addr;
	  if (addr > max_addr)
	    max_addr = addr;
	}
      
      task_symbols[task].beg = min_addr;
      task_symbols[task].end = max_addr;
    }
}

