IMPLEMENTATION[ia32]:

#include <cstdio>
#include <cstring>
#include <flux/x86/base_idt.h>
#include <flux/x86/tss.h>

#include "config.h"
#include "globals.h"
#include "io.h"
#include "pic.h"
#include "regdefs.h"
#include "space.h"
#include "kmem.h"
#include "jdb_perf_cnt.h"
#include "jdb_symbol.h"


PRIVATE
void Jdb_kern_info::show_pic_state()
{
  int i;
  static char const hex[] = "0123456789ABCDEF";
	  
  // show important I/O ports
  Io::out8_p(Pic::OCW_TEMPLATE | Pic::READ_NEXT_RD | Pic::READ_IS_ONRD, 
	     Pic::MASTER_ICW );
  unsigned in_service = Io::in8(Pic::MASTER_ICW);
  Io::out8_p(Pic::OCW_TEMPLATE | Pic::READ_NEXT_RD | Pic::READ_IR_ONRD,
	     Pic::MASTER_ICW);
  unsigned requested = Io::in8(Pic::MASTER_ICW);
  unsigned mask = Jdb::pic_status & 0x0ff;
  printf("master PIC: in service:");
  for (i=7; i>=0; i--)
    putchar((in_service & (1<<i)) ? hex[i] : '-');
  printf(", request:");
  for (i=7; i>=0; i--)
    putchar((requested & (1<<i)) ? hex[i] : '-');
  printf(", mask:");
  for (i=7; i>=0; i--)
    putchar((mask & (1<<i)) ? hex[i] : '-');
  putchar('\n');
	  
  Io::out8_p( Pic::OCW_TEMPLATE | Pic::READ_NEXT_RD | Pic::READ_IS_ONRD, 
	      Pic::SLAVES_ICW);
  in_service = Io::in8(Pic::SLAVES_ICW);
  Io::out8_p( Pic::OCW_TEMPLATE | Pic::READ_NEXT_RD | Pic::READ_IR_ONRD, 
	      Pic::SLAVES_ICW);
  requested = Io::in8(Pic::SLAVES_ICW);
  mask = Jdb::pic_status >> 8;
  printf(" slave PIC: in service:");
  for (i=7; i>=0; i--)
    putchar((in_service & (1<<i)) ? hex[i+8] : '-');
  printf(", request:");
  for (i=7; i>=0; i--)
    putchar((requested & (1<<i)) ? hex[i+8] : '-');
  printf(", mask:");
  for (i=7; i>=0; i--)
    putchar((mask & (1<<i)) ? hex[i+8] : '-');
  putchar('\n');
}


// missing accessor macros in oskit/flux/x86/proc_reg.h

#define get_idt(pseudo_desc) \
    ({ \
	asm volatile("sidt %0" : "=m" ((pseudo_desc)->limit) : : "memory"); \
    })

#define get_gdt(pseudo_desc) \
    ({ \
	asm volatile("sgdt %0" : "=m" ((pseudo_desc)->limit) : : "memory"); \
    })

PRIVATE
void Jdb_kern_info::show_x86_desc(x86_desc *d)
{
  static char const * const desc_code_data_type[16] = 
    { "data r/o",          "data r/o acc",
      "data r/w",          "data r/w acc",
      "data r/o exp-dn",   "data r/o exp-dn",
      "data r/w exp-dn",   "data r/w exp-dn acc",
      "code x/o",          "code x/o acc", 
      "code x/r",          "code x/r acc",
      "code x/r conf",     "code x/o conf acc",
      "code x/r conf",     "code x/r conf acc" };
  static char const * const desc_system_type[16] =
    { "reserved",          "16-bit tss (avail)",
      "ldt",               "16-bit tss (busy)",
      "16-bit call gate",  "task gate",
      "16-bit int gate",   "16-bit trap gate",
      "reserved",          "32-bit tss (avail)",
      "reserved",          "32-bit tss (busy)",
      "32-bit call gate",  "reserved",
      "32-bit int gate",   "32-bit trap gate" };
  if ((d->access & 0x16) == 0x6)
    {
      // interrupt/trap gate
      x86_gate *g = reinterpret_cast<x86_gate*>(d);
      Address offset = (g->offset_low | (g->offset_high << 16));
      const char *symbol;
      printf("%08x  dpl=%d (\033[33;1m%s\033[0m)",
	      offset,
	      (d->access & ACC_PL) >> 5,
	      ((d->access & 0x7) == 6) ? "int gate " : "trap gate");
      if ((symbol = Jdb_symbol::match_address_to_symbol(offset, 0)))
	{
	  char str[40];
	  strncpy(str, symbol, sizeof(str)-1);
	  str[sizeof(str)-1] = '\0';
	  printf(" : %s", str);
	}
      putchar('\n');
    }
  else
    {
      // segment descriptor
      Address base  = (d->base_high  << 24) | (d->base_med << 16) | d->base_low;
      Address limit = (d->limit_high << 16) |  d->limit_low;
      printf("%08x-%08x dpl=%d %dbit %s %02X (\033[33;1m%s\033[0m)\n",
	    base, (d->granularity & SZ_G) == 0
		     ? base + limit
		     : base + ((limit+1) << 12)-1,
	    (d->access & ACC_PL) >> 5,
	    (d->granularity & SZ_32) ? 32 : 16,
    	    (d->access & ACC_TYPE_USER) ? "code/data" : "system   ",
	    d->access & 0xe, 
	    (d->access & ACC_TYPE_USER) 
		? desc_code_data_type[d->access & 0xf]
		: desc_system_type[d->access & 0xf]);
    }
}

PRIVATE
void Jdb_kern_info::show_misc_info()
{
  Space *s = current_space();
  pseudo_descriptor gdt_pseudo, idt_pseudo;
  
  get_gdt(&gdt_pseudo);
  get_idt(&idt_pseudo);
  printf("clck: %08x.%08x\n"
	 "pdir: %08x (taskno=%03x, chief=%03x)\n"
	 "idt : base=%08lx  limit=%04x\n"
	 "gdt : base=%08lx  limit=%04x\n",
	 (unsigned) (Kmem::info()->clock >> 32), 
	 (unsigned) (Kmem::info()->clock),
	 (unsigned) s,
	 unsigned(s->space()),  unsigned(s->chief()),
	 idt_pseudo.linear_base, (idt_pseudo.limit+1)/8,
	 gdt_pseudo.linear_base, (gdt_pseudo.limit+1)/8);

  // print LDT
  printf("LDT :  %04x", get_ldt());
  if(get_ldt() != 0)
    {
      printf(":\n       ");
      show_x86_desc(Kmem::gdt + (get_ldt() >> 3));
    }
  else
    putchar('\n');

  // print TSS
  printf("TR  :  %04x", get_tr());
  if(get_tr() != 0)
    {
      x86_desc *d = Kmem::gdt + (get_tr() >> 3);
      printf(":\n       ");
      show_x86_desc(d);
      Address base = (d->base_high << 24) | (d->base_med << 16) | d->base_low;
      printf("       i/o bitmap at %08x\n",
	     base + (reinterpret_cast<x86_tss *>(base))->io_bit_map_offset);
    }
  else
    putchar('\n');

  printf("CR0 :  %08x\n"
	 "CR4 :  %08x\n",
	 get_cr0(), get_cr4());
}


PRIVATE
void Jdb_kern_info::show_gdt_idt()
{
  pseudo_descriptor gdt_pseudo, idt_pseudo;
  int i;

  get_gdt(&gdt_pseudo);
  get_idt(&idt_pseudo);
  printf("gdt base=%08lx  limit=%04x (%04x bytes)\n",
	 gdt_pseudo.linear_base, (gdt_pseudo.limit+1)/8, gdt_pseudo.limit+1);
  x86_desc *gdt = reinterpret_cast<x86_desc*>(gdt_pseudo.linear_base);
  for (i=0; i<(gdt_pseudo.limit+1)/8; i++)
    {
      if (gdt[i].access & ACC_P)
	{
	  printf("%3x: ",i);
	  show_x86_desc(gdt + i);
	}
    }

  printf("\nidt base=%08lx  limit=%04x (%04x bytes)\n",
	 idt_pseudo.linear_base, (idt_pseudo.limit+1)/8, idt_pseudo.limit+1);
  x86_desc *idt = reinterpret_cast<x86_desc*>(idt_pseudo.linear_base);
  for (i=0; i<(idt_pseudo.limit+1)/8; i++)
    {
      if (idt[i].access & ACC_P)
	{
	  printf("%3x: ",i);
	  show_x86_desc(idt + i);
	}
    }
}

PRIVATE
unsigned
Jdb_kern_info::show_feature_bits (unsigned features, const char *const *table,
				  unsigned first_pos, unsigned last_pos)
{
  unsigned i, count, len, colon;

  for (i=count=colon=0, len=last_pos; *table != (char *) -1; i++, table++)
    if ((features & (1 << i)) && *table)
      {
	printf ("%s%s", colon ? "," : "", *table);
	len += strlen(*table) + colon;
	colon = 1;
	if (len > 50)
	  {
	    colon = 0;
	    len = first_pos;
	    printf("\n%*s", first_pos, "");
	  }
      }

  return len;
}

PRIVATE
void Jdb_kern_info::show_cpu_info()
{
  const char *const simple[] = 
  {
    "fpu", "vme", "de", "pse", "tsc", "msr", "pae", "mce", "cx8", "apic",
    NULL, "sep", "mtrr", "pge", "mca", "cmov", "pat", "pse36", "psn",
    "clfsh", NULL, "ds", "acpi", "mmx", "fxsr", "sse", "sse2", "ss",
    "htt", "tm", NULL, "pbe", (char *)(-1)
  };
  const char *const extended[] =
  {
    "pni", NULL, NULL, "monitor", "dscpl", NULL, NULL, NULL, "tm2", NULL,
    "cnxtid", (char *)(-1)
  };

  const char *perf_type = Jdb_perf_cnt::perf_type();
  char cpu_mhz[32];
  char apic_state[80];
  unsigned hz, khz;

  cpu_mhz[0] = '\0';
  if ((hz = Cpu::hz()))
    {
      unsigned mhz = hz / 1000000;
      hz -= mhz * 1000000;
      unsigned khz = hz / 1000;
      sprintf(cpu_mhz, "%d.%03d MHz", mhz, khz);
    }

  strcpy (apic_state, "N/A");
  if ((khz = Apic::get_frequency_khz()))
    {
      unsigned mhz = khz / 1000;
      khz -= mhz * 1000;
      sprintf(apic_state, " %d.%03d MHz"
	      "\n  local APIC spurious interrupts/bug/error: %d/%d/%d",
	      mhz, khz,
	      apic_spurious_interrupt_cnt,
	      apic_spurious_interrupt_bug_cnt,
	      apic_error_cnt);
    }

  printf ("CPU: %s %s (%s)\n",
          Cpu::model_str(), cpu_mhz,
          Config::found_vmware ? "vmware" : "native");

  putstr ("     ");
  unsigned position = 0;
  position = show_feature_bits (Cpu::features(), simple, 5, position);
  position = show_feature_bits (Cpu::ext_features(), extended, 5, position);

  printf("\nLocal APIC state: %s"
	 "\nTimer interrupt source: %s"
	 "\nPerformance counters: %s"
	 "\nLast branch recording: %s"
	 "\nWatchdog: %s"
	 "\n",
	 apic_state,
	 Config::scheduling_using_pit ? "PIT" : "RTC",
	 perf_type ? perf_type : "no",
	 Cpu::lbr_type() != Cpu::LBR_NONE 
	 ? Cpu::lbr_type() == Cpu::LBR_P4 ? "P4" : "P6"
	 : "no",
	 Config::watchdog 
	 ? "active" 
	 : Apic::is_present()
	 ? "disabled"
         : "not supported"
	 );
}
