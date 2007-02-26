IMPLEMENTATION:

#include <cstdio>

#include "apic.h"
#include "cpu.h"
#include "jdb.h"
#include "jdb_module.h"
#include "jdb_tbuf.h"
#include "static_init.h"
#include "kmem_alloc.h"
#include "region.h"


//===================
// Std JDB modules
//===================

/**
 * @brief 'kern info' module.
 * 
 * This module handles the 'k' command, which
 * prints out various kernel information.
 */
class Jdb_kern_info 
  : public Jdb_module
{
public:
private:
  char subcmd;
};

static Jdb_kern_info jdb_kern_info INIT_PRIORITY(JDB_MODULE_INIT_PRIO);

PUBLIC
Jdb_module::Action_code Jdb_kern_info::action( int cmd, void *&args, char const *& )
{
  if(cmd!=0)
    return NOTHING;
    
  switch(*(char*)(args))
    {
    case 'p': show_pic_state(); break;
    case 'i': show_misc_info(); break;
    case 'c': show_cpu_info(); break;
    case 'g': show_gdt_idt(); break;
    case 'm': Kmem_alloc::allocator()->debug_dump(); break;
    case 'r': region::debug_dump();  break;
    case 'f': Kmem::info()->print(); break;
    default:
      printf("  kc - CPU features\n"
	     "  kg - show GDT and IDT\n"
	     "  kf - show kernel interface page\n"
             "  ki - miscellanous info\n"
             "  km - kmem_alloc::debug_dump\n"
	     "  kp - PIC ports\n"
	     "  kr - region::debug_dump\n");
      break;
      
    }

  putchar('\n');
  return NOTHING;
}

PUBLIC
int const Jdb_kern_info::num_cmds() const
{ 
  return 1;
}

PUBLIC
Jdb_module::Cmd const *const Jdb_kern_info::cmds() const
{
  static Cmd cs[] =
    { Cmd( 0, "k", "k", "%c\n", 
	   "k{c|g|f|i|m|p|r}\tshow various kernel information (kh=help)", (void*)&subcmd )
    };

  return cs;
}

PUBLIC
Jdb_kern_info::Jdb_kern_info()
	: Jdb_module("INFO")
{}
