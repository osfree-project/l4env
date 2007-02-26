/*
 * Fiasco CPU Code
 * Shared between UX and native IA32.
 */

INTERFACE:

#include "types.h"

class Cpu
{
public:

  enum Vendor {
    VENDOR_UNKNOWN = 0,
    VENDOR_INTEL,
    VENDOR_AMD,
    VENDOR_CYRIX,
    VENDOR_IDT,
    VENDOR_UMC,
    VENDOR_NEXGEN,
    VENDOR_RISE,
    VENDOR_TRANSMETA,
  };

  enum CacheTLB {
    CACHE_UNKNOWN = 0,
    CACHE_L1_DATA,
    CACHE_L1_INST,
    CACHE_L1_TRACE,
    CACHE_L2,
    CACHE_L3,
    TLB_DATA,
    TLB_INST
  };

  static void			init();
  static char const*		vendor_str();
  static char const*		model_str();
  static Cpu::Vendor const	vendor();
  static unsigned int const 	family();
  static unsigned int const 	model();
  static unsigned int const 	stepping();
  static Unsigned64		frequency();
  static unsigned int const 	apic();
  static unsigned int const 	features();
  static unsigned int const	ext_features();
  static void			identify();

  static unsigned short const	inst_tlb_entries();
  static unsigned short const	data_tlb_entries();

  static unsigned short const  	l1_trace_cache_size();
  static unsigned short const 	l1_trace_cache_asso();

  static unsigned short const 	l1_data_cache_size();
  static unsigned short const 	l1_data_cache_asso();
  static unsigned short const 	l1_data_cache_line_size();

  static unsigned short const 	l1_inst_cache_size();
  static unsigned short const 	l1_inst_cache_asso();
  static unsigned short const 	l1_inst_cache_line_size();

  static unsigned short const 	l2_cache_size();
  static unsigned short const 	l2_cache_asso();
  static unsigned short const	l2_cache_line_size();

  static Unsigned64		time_us();
  static Unsigned64		rdtsc();
  static Unsigned64		ns_to_tsc(Unsigned64 ns);
  static Unsigned64		tsc_to_us(Unsigned64 tsc);
  static Unsigned64		tsc_to_ns(Unsigned64 tsc);
  static void			tsc_to_s_and_ns(Unsigned64 tsc, 
						Unsigned32 *s, Unsigned32 *ns);
  static Unsigned32		get_scaler_tsc_to_ns();
  static Unsigned32		get_scaler_tsc_to_us();
  static Unsigned32		get_scaler_ns_to_tsc();

private:

  struct Vendor_table {
    unsigned			vendor_mask;
    unsigned			vendor_code;
    unsigned short		l2_cache;
    char			vendor_string[32];
  };

  struct Cache_table {
    unsigned char		desc;
    Cpu::CacheTLB		level;
    unsigned short		size;
    unsigned short		asso;
    unsigned short		line_size;
  };

  static unsigned		cpu_version;
  static Unsigned64		cpu_frequency;
  static unsigned		cpu_apic;
  static unsigned		cpu_features;
  static unsigned		cpu_ext_features;

  static unsigned short		cpu_inst_tlb_entries;
  static unsigned short		cpu_data_tlb_entries;

  static unsigned short		cpu_l1_trace_cache_size;
  static unsigned short		cpu_l1_trace_cache_asso;

  static unsigned short		cpu_l1_data_cache_size;
  static unsigned short		cpu_l1_data_cache_asso;
  static unsigned short		cpu_l1_data_cache_line_size;

  static unsigned short		cpu_l1_inst_cache_size;
  static unsigned short		cpu_l1_inst_cache_asso;
  static unsigned short		cpu_l1_inst_cache_line_size;

  static unsigned short		cpu_l2_cache_size;
  static unsigned short		cpu_l2_cache_asso;
  static unsigned short		cpu_l2_cache_line_size;

  static Vendor			cpu_vendor;
  static char   	        cpu_vendor_str[13];
  static char			cpu_model_str[32];

  static Vendor_table const	intel_table[];
  static Vendor_table const	amd_table[];
  static Vendor_table const	cyrix_table[];
  static Vendor_table const	idt_table[];
  static Cache_table const	intel_cache_table[];

  static void			cpuid (unsigned const mode,
				       unsigned *const eax,
				       unsigned *const ebx,
				       unsigned *const ecx,
				       unsigned *const edx);

  static void			cache_tlb_intel();
  static void			cache_tlb_amd_l1();
  static void			cache_tlb_amd_l2();
  static void			set_model_str (Cpu::Vendor const vendor,
	                                       unsigned const version,
        	                               unsigned short const l2_cache);

  static Unsigned32		scaler_tsc_to_ns;
  static Unsigned32		scaler_tsc_to_us;
  static Unsigned32		scaler_ns_to_tsc;
};

IMPLEMENTATION[ia32-common]:

#include <cstdio>
#include <cstring>
#include <flux/x86/proc_reg.h>
#include "panic.h"
#include "regdefs.h"
#include "initcalls.h"

unsigned        Cpu::cpu_version;
Unsigned64	Cpu::cpu_frequency;
unsigned        Cpu::cpu_apic;
unsigned        Cpu::cpu_features;
unsigned	Cpu::cpu_ext_features;

unsigned short	Cpu::cpu_inst_tlb_entries;
unsigned short	Cpu::cpu_data_tlb_entries;

unsigned short	Cpu::cpu_l1_trace_cache_size;
unsigned short	Cpu::cpu_l1_trace_cache_asso;

unsigned short	Cpu::cpu_l1_data_cache_size;
unsigned short	Cpu::cpu_l1_data_cache_asso;
unsigned short	Cpu::cpu_l1_data_cache_line_size;

unsigned short	Cpu::cpu_l1_inst_cache_size;
unsigned short	Cpu::cpu_l1_inst_cache_asso;
unsigned short	Cpu::cpu_l1_inst_cache_line_size;

unsigned short	Cpu::cpu_l2_cache_size;
unsigned short	Cpu::cpu_l2_cache_asso;
unsigned short	Cpu::cpu_l2_cache_line_size;

Cpu::Vendor     Cpu::cpu_vendor;
char            Cpu::cpu_vendor_str[13];
char		Cpu::cpu_model_str[32];
  
Cpu::Vendor_table const Cpu::intel_table[] FIASCO_INITDATA =
{
  { 0xFF0,	0x400,	0xFFFF,	"i486 DX-25/33"				},
  { 0xFF0,	0x410,	0xFFFF,	"i486 DX-50"				},
  { 0xFF0,	0x420,	0xFFFF,	"i486 SX"				},
  { 0xFF0,	0x430,	0xFFFF,	"i486 DX/2"				},
  { 0xFF0,	0x440,	0xFFFF,	"i486 SL"				},
  { 0xFF0,	0x450,	0xFFFF,	"i486 SX/2"				},
  { 0xFF0,	0x470,	0xFFFF,	"i486 DX/2-WB"				},
  { 0xFF0,	0x480,	0xFFFF,	"i486 DX/4"				},
  { 0xFF0,	0x490,	0xFFFF,	"i486 DX/4-WB"				},
  { 0xFF0,	0x500,	0xFFFF,	"Pentium A-Step"			},
  { 0xFF0,	0x510,	0xFFFF,	"Pentium 60-66"				},
  { 0xFF0,	0x520,	0xFFFF,	"Pentium 75-200"			},
  { 0xFF0,	0x530,	0xFFFF,	"Pentium Overdrive"			},
  { 0xFF0,	0x540,	0xFFFF,	"Pentium MMX"				},
  { 0xFF0,	0x570,	0xFFFF,	"Pentium Mobile"			},
  { 0xFF0,	0x580,	0xFFFF,	"Pentium MMX Mobile (Tillamook)"	},
  { 0xFF0,	0x600,	0xFFFF,	"Pentium-Pro A-Step"			},
  { 0xFF0,	0x610,	0xFFFF,	"Pentium-Pro"				},
  { 0xFF0,	0x630,	512,	"Pentium II (Klamath)"			},
  { 0xFF0,	0x640,	512,	"Pentium II (Deschutes)"		},
  { 0xFF0,	0x650,	1024,	"Pentium II (Drake)"			},
  { 0xFF0,	0x650,	512,	"Pentium II (Deschutes)"		},
  { 0xFF0,	0x650,	256,	"Pentium II Mobile (Dixon)"		},
  { 0xFF0,	0x650,	0,	"Celeron (Covington)"			},
  { 0xFF0,	0x660,	128,	"Celeron (Mendocino)"			},
  { 0xFF0,	0x670,	1024,	"Pentium III (Tanner)"			},
  { 0xFF0,	0x670,	512,	"Pentium III (Katmai)"			},
  { 0xFF0,	0x680,	256,	"Pentium III (Coppermine)"		},
  { 0xFF0,	0x680,	128,	"Celeron (Coppermine)"			},
  { 0xFF0,	0x690,	1024,	"Pentium-M (Banias)"			},
  { 0xFF0,	0x6a0,	1024,	"Pentium III (Cascades)"		},
  { 0xFF0,	0x6b0,	512,	"Pentium III-S"				},
  { 0xFF0,	0x6b0,	256,	"Pentium III (Tualatin)"		},
  { 0xF00,	0x700,	0xFFFF,	"Itanium"				},
  { 0xFF0,	0xf00,	256,	"Pentium 4 (Willamette/Foster)"		},
  { 0xFF0,	0xf10,	256,	"Pentium 4 (Willamette/Foster)"		},
  { 0xFF0,	0xf10,	128,	"Celeron (Willamette)"			},
  { 0xFF0,	0xf20,	512,	"Pentium 4 (Northwood/Prestonia)"	},
  { 0xFF0,	0xf20,	128,	"Celeron (Northwood)"			},
  { 0xFF0,	0xf30,	0xFFFF, "Pentium 4 (Prescott/Nocona)"		},
  { 0x0,	0x0,	0xFFFF,	""					}
};

Cpu::Vendor_table const Cpu::amd_table[] FIASCO_INITDATA =
{
  { 0xFF0,	0x430,	0xFFFF,	"Am486DX2-WT"			},
  { 0xFF0,	0x470,	0xFFFF,	"Am486DX2-WB"			},
  { 0xFF0,	0x480,	0xFFFF,	"Am486DX4-WT / Am5x86-WT"	},
  { 0xFF0,	0x490,	0xFFFF,	"Am486DX4-WB / Am5x86-WB"	},
  { 0xFF0,	0x4a0,	0xFFFF,	"SC400"				},
  { 0xFF0,	0x4e0,	0xFFFF,	"Am5x86-WT"			},
  { 0xFF0,	0x4f0,	0xFFFF,	"Am5x86-WB"			},
  { 0xFF0,	0x500,	0xFFFF,	"SSA5 (PR75/PR90/PR100)"	},
  { 0xFF0,	0x510,	0xFFFF,	"K5 (PR120/PR133)"		},
  { 0xFF0,	0x520,	0xFFFF,	"K5 (PR166)"			},
  { 0xFF0,	0x530,	0xFFFF,	"K5 (PR200)"			},
  { 0xFF0,	0x560,	0xFFFF,	"K6 (0.30 um)"			},
  { 0xFF0,	0x570,	0xFFFF,	"K6 (0.25 um)"			},
  { 0xFF0,	0x580,	0xFFFF,	"K6-II"				},
  { 0xFF0,	0x590,	0xFFFF,	"K6-III (Sharptooth)"		},
  { 0xFF0,	0x5c0,	0xFFFF,	"K6-2+"				},
  { 0xFF0,	0x5d0,	0xFFFF,	"K6-3+"				},
  { 0xFF0,	0x600,	0xFFFF,	"K7 ES"				},
  { 0xFF0,	0x610,	0xFFFF,	"Athlon K7 (Argon)"		},
  { 0xFF0,	0x620,	0xFFFF,	"Athlon K75 (Orion)"		},
  { 0xFF0,	0x630,	0xFFFF,	"Duron (Spitfire)"		},
  { 0xFF0,	0x640,	0xFFFF,	"Athlon (Thunderbird)"		},
  { 0xFF0,	0x660,	256,	"Athlon (Palomino)"		},
  { 0xFF0,	0x660,	64,	"Duron (Morgan)"		},
  { 0xFF0,	0x670,	0xFFFF,	"Duron (Morgan)"		},
  { 0xFF0,	0x680,	256,	"Athlon (Thoroughbred)"		},
  { 0xFF0,	0x680,	64,	"Duron (Applebred)"		},
  { 0xFF0,	0x6A0,	0xFFFF,	"Athlon (Barton)"		},
  { 0xFF0,	0xf40,	0xFFFF,	"Athlon 64 (Clawhammer)"	},
  { 0xFF0,	0xf50,	0xFFFF,	"Opteron (Sledgehammer)"	},
  { 0x0,	0x0,	0xFFFF,	""				}
};

Cpu::Vendor_table const Cpu::cyrix_table[] FIASCO_INITDATA =
{
  { 0xFF0,	0x450,	0xFFFF,	"Media GX"			},
  { 0xFFF,	0x524,	0xFFFF,	"GXm"				},
  { 0xFF0,	0x520,	0xFFFF,	"6x86"				},
  { 0xFF0,	0x600,	0xFFFF,	"6x86/MX"			},
  { 0xFF0,	0x620,	0xFFFF,	"MII"				},
  { 0x0,	0x0,	0xFFFF,	""				}
};

Cpu::Vendor_table const Cpu::idt_table[] FIASCO_INITDATA =
{
  { 0xFF0,	0x540,	0xFFFF,	"Winchip C6"			},
  { 0xFF0,	0x580,	0xFFFF,	"Winchip 2A/B"			},
  { 0xFF0,	0x590,	0xFFFF,	"Winchip 3"			},
  { 0xFF0,	0x660,	0xFFFF,	"Via C3 (Samuel)"		},
  { 0xFF0,	0x670,	0xFFFF,	"Via C3 (Ezra)"			},
  { 0xFF0,	0x680,	0xFFFF,	"Via C3 (Ezra-T)"		},
  { 0xFF0,	0x690,	0xFFFF,	"Via C3 (Nehemiah)"		},
  { 0x0,	0x0,	0xFFFF,	""				}
};

Cpu::Cache_table const Cpu::intel_cache_table[] FIASCO_INITDATA =
{
  { 0x01,	TLB_INST,	32,	4,	0	},
  { 0x03,	TLB_DATA,	64,	4,	0	},
  { 0x06,	CACHE_L1_INST,	8,	4,	32	},
  { 0x08,	CACHE_L1_INST,	16,	4,	32	},
  { 0x0A,	CACHE_L1_DATA,	8,	2,	32	},
  { 0x0C,	CACHE_L1_DATA,	16,	4,	32	},
  { 0x22,	CACHE_L3,	512,	4,	64	}, /* sectored */
  { 0x23,	CACHE_L3,	1024,	8,	64	}, /* sectored */
  { 0x25,	CACHE_L3,	2048,	8,	64	}, /* sectored */
  { 0x29,	CACHE_L3,	4096,	8,	64	}, /* sectored */
  { 0x2C,	CACHE_L1_DATA,	32,	8,	64	},
  { 0x30,	CACHE_L1_INST,	32,	8,	64	},
  { 0x39,	CACHE_L2,	128,	4,	64	}, /* sectored */
  { 0x3B,	CACHE_L2,	256,	2,	64	}, /* sectored */
  { 0x3C,	CACHE_L2,	256,	4,	64	}, /* sectored */
  { 0x41,	CACHE_L2,	128,	4,	32	},
  { 0x42,	CACHE_L2,	256,	4,	32	},
  { 0x43,	CACHE_L2,	512,	4,	32	},
  { 0x44,	CACHE_L2,	1024,	4,	32	},
  { 0x45,	CACHE_L2,	2048,	4,	32	},
  { 0x50,	TLB_INST,	64,	0,	0	},
  { 0x51,	TLB_INST,	128,	0,	0	},
  { 0x52,	TLB_INST,	256,	0,	0	},
  { 0x5B,	TLB_DATA,	64,	0,	0	},
  { 0x5C,	TLB_DATA,	128,	0,	0	},
  { 0x5D,	TLB_DATA,	256,	0,	0	},
  { 0x66,	CACHE_L1_DATA,	8,	4,	64	}, /* sectored */
  { 0x67,	CACHE_L1_DATA,	16,	4,	64	}, /* sectored */
  { 0x68,	CACHE_L1_DATA,	32,	4,	64	}, /* sectored */
  { 0x70,	CACHE_L1_TRACE,	12,	8,	0	},
  { 0x71,	CACHE_L1_TRACE,	16,	8,	0	},
  { 0x72,	CACHE_L1_TRACE,	32,	8,	0	},
  { 0x77,	CACHE_L1_INST,  16,	4,	64	},
  { 0x79,	CACHE_L2,	128,	8,	64	}, /* sectored */
  { 0x7A,	CACHE_L2,	256,	8,	64	}, /* sectored */
  { 0x7B,	CACHE_L2,	512,	8,	64	}, /* sectored */
  { 0x7C,	CACHE_L2,	1024,	8,	64	}, /* sectored */
  { 0x7E,	CACHE_L2,	256,	8,	128	},
  { 0x82,	CACHE_L2,	256,	8,	32	},
  { 0x83,	CACHE_L2,	512,	8,	32	},
  { 0x84,	CACHE_L2,	1024,	8,	32	},
  { 0x85,	CACHE_L2,	2048,	8,	32	},
  { 0x86,	CACHE_L2,	512,	4,	64	},
  { 0x87,	CACHE_L2,	1024,	8,	64	},
  { 0x8D,	CACHE_L3,	3072,	12,	128	},
  { 0xB0,	TLB_INST,	128,	4,	0	},
  { 0xB3,	TLB_DATA,	128,	4,	0	},
  { 0x0,	CACHE_UNKNOWN,	0,	0,	0	}
};

Unsigned32	Cpu::scaler_tsc_to_ns;
Unsigned32	Cpu::scaler_tsc_to_us;
Unsigned32	Cpu::scaler_ns_to_tsc;


IMPLEMENT inline
void
Cpu::cpuid (unsigned const mode,
            unsigned *const eax, unsigned *const ebx,
	    unsigned *const ecx, unsigned *const edx)
{
  asm volatile ("cpuid" : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
                        : "a" (mode));
}

IMPLEMENT inline
char const*
Cpu::vendor_str()
{
  return cpu_vendor_str;
}

IMPLEMENT inline
char const*
Cpu::model_str()
{
  return cpu_model_str;
}

IMPLEMENT inline
Cpu::Vendor const
Cpu::vendor()
{
  return cpu_vendor;
}

IMPLEMENT inline
unsigned int const
Cpu::family()
{
  unsigned int val = (cpu_version >> 8) & 0xF;

  return val == 0xF ? val + ((cpu_version >> 16) & 0xF0) : val;
}

IMPLEMENT inline
unsigned int const
Cpu::model()
{
  unsigned int val = (cpu_version >> 4) & 0xF;

  return val == 0xF ? val + ((cpu_version >> 12) & 0xF0) : val;
}

IMPLEMENT inline
unsigned int const
Cpu::stepping()
{
  return cpu_version & 0xF;
}

IMPLEMENT inline
Unsigned64
Cpu::frequency()
{
  return cpu_frequency;
}

IMPLEMENT inline
unsigned int const
Cpu::apic()
{
  return (cpu_apic >> 24) & 0xFF;
}

IMPLEMENT inline
unsigned int const
Cpu::features()
{
  return cpu_features;
}

IMPLEMENT inline
unsigned int const
Cpu::ext_features()
{
  return cpu_ext_features;
}

IMPLEMENT inline
unsigned short const
Cpu::inst_tlb_entries()
{
  return cpu_inst_tlb_entries;
}

IMPLEMENT inline
unsigned short const
Cpu::data_tlb_entries()
{
  return cpu_data_tlb_entries;
}

IMPLEMENT inline
unsigned short const
Cpu::l1_trace_cache_size()
{
  return cpu_l1_trace_cache_size;
}

IMPLEMENT inline
unsigned short const
Cpu::l1_trace_cache_asso()
{
  return cpu_l1_trace_cache_asso;
}

IMPLEMENT inline
unsigned short const
Cpu::l1_data_cache_size()
{
  return cpu_l1_data_cache_size;
}

IMPLEMENT inline
unsigned short const
Cpu::l1_data_cache_asso()
{
  return cpu_l1_data_cache_asso;
}

IMPLEMENT inline
unsigned short const
Cpu::l1_data_cache_line_size()
{
  return cpu_l1_data_cache_line_size;
}

IMPLEMENT inline
unsigned short const
Cpu::l1_inst_cache_size()
{
  return cpu_l1_inst_cache_size;
}

IMPLEMENT inline
unsigned short const
Cpu::l1_inst_cache_asso()
{
  return cpu_l1_inst_cache_asso;
}

IMPLEMENT inline
unsigned short const
Cpu::l1_inst_cache_line_size()
{
  return cpu_l1_inst_cache_line_size;
}

IMPLEMENT inline
unsigned short const
Cpu::l2_cache_size()
{
  return cpu_l2_cache_size;
}

IMPLEMENT inline
unsigned short const
Cpu::l2_cache_asso()
{
  return cpu_l2_cache_asso;
}

IMPLEMENT inline
unsigned short const
Cpu::l2_cache_line_size()
{
  return cpu_l2_cache_line_size;
}

IMPLEMENT FIASCO_INIT
void
Cpu::cache_tlb_intel()
{
  unsigned char desc[16];
  unsigned i, count = 0;
  Cache_table const *table;

  do {

    cpuid (2, (unsigned int *)(desc),
              (unsigned int *)(desc + 4),
              (unsigned int *)(desc + 8),
              (unsigned int *)(desc + 12));

    for (i = 1; i < 16; i++) {

      // Null descriptor or register bit31 set (reserved)
      if (!desc[i] || (desc[i / 4 * 4 + 3] & (1 << 7)))	
        continue;

      for (table = intel_cache_table; table->desc; table++) {
        if (table->desc == desc[i]) {
          switch (table->level) {
            case CACHE_L1_DATA:
              cpu_l1_data_cache_size      = table->size;
              cpu_l1_data_cache_asso      = table->asso;
              cpu_l1_data_cache_line_size = table->line_size;
              break;
            case CACHE_L1_INST:
              cpu_l1_inst_cache_size      = table->size;
              cpu_l1_inst_cache_asso      = table->asso;
              cpu_l1_inst_cache_line_size = table->line_size;
              break;
            case CACHE_L1_TRACE:
              cpu_l1_trace_cache_size = table->size;
              cpu_l1_trace_cache_asso = table->asso;
              break;
            case CACHE_L2:
              cpu_l2_cache_size      = table->size;
              cpu_l2_cache_asso      = table->asso;
              cpu_l2_cache_line_size = table->line_size;
              break;
            case TLB_INST:
              cpu_inst_tlb_entries += table->size;
              break;
            case TLB_DATA:
              cpu_data_tlb_entries += table->size;
              break;
            default:
              break;
          }
          break;
        }
      }
    }
  } while (++count < *desc);
}

IMPLEMENT FIASCO_INIT
void
Cpu::cache_tlb_amd_l1()
{
  unsigned eax, ebx, ecx, edx;
  cpuid (0x80000005, &eax, &ebx, &ecx, &edx);

  cpu_l1_data_cache_size = (ecx >> 24) & 0xFF;
  cpu_l1_data_cache_asso = (ecx >> 16) & 0xFF;
  cpu_l1_data_cache_line_size = ecx & 0xFF;

  cpu_l1_inst_cache_size = (edx >> 24) & 0xFF;
  cpu_l1_inst_cache_asso = (edx >> 16) & 0xFF;
  cpu_l1_inst_cache_line_size = edx & 0xFF;

  cpu_inst_tlb_entries += ebx & 0xFF;
  cpu_data_tlb_entries += (ebx >> 16) & 0xFF;
}

IMPLEMENT FIASCO_INIT
void
Cpu::cache_tlb_amd_l2()
{
  unsigned eax, ebx, ecx, edx;
  cpuid (0x80000006, &eax, &ebx, &ecx, &edx);

  cpu_l2_cache_size = (ecx >> 16) & 0xFFFF;
  cpu_l2_cache_asso = (ecx >> 12) & 0xF;
  cpu_l2_cache_line_size = ecx & 0xFF;
}

IMPLEMENT FIASCO_INIT
void
Cpu::set_model_str (Cpu::Vendor const vendor, unsigned const version,
                    unsigned short const l2_cache)
{
  Vendor_table const *table;

  switch (vendor) {
    case VENDOR_INTEL:	table = intel_table;	break;
    case VENDOR_AMD:	table = amd_table;	break;
    case VENDOR_CYRIX:	table = cyrix_table;	break;
    case VENDOR_IDT:	table = idt_table;	break;
    default:		table = NULL;		break;
  }

  for (; table && table->vendor_mask; table++)
    if ((version & table->vendor_mask) == table->vendor_code &&
        (table->l2_cache == 0xFFFF || l2_cache >= table->l2_cache))
      {
	snprintf (cpu_model_str, sizeof (cpu_model_str), "%s",
		  table->vendor_string);
	return;
      }

  snprintf (cpu_model_str, sizeof (cpu_model_str), "Unknown CPU");
}

IMPLEMENT FIASCO_INIT
void
Cpu::identify()
{
  unsigned eflags = get_eflags();

  // Check for Alignment Check Support
  set_eflags (eflags ^ EFLAGS_AC);
  if (((get_eflags() ^ eflags) & EFLAGS_AC) == 0)
    panic ("CPU too old");

  // Check for CPUID Support
  set_eflags (eflags ^ EFLAGS_ID);
  if ((get_eflags() ^ eflags) & EFLAGS_ID) {

    unsigned max, dummy;

    cpuid (0, &max, (unsigned int *)(cpu_vendor_str),
                    (unsigned int *)(cpu_vendor_str + 8),
                    (unsigned int *)(cpu_vendor_str + 4));

    cpu_vendor_str[12] = 0;       // Terminate Vendor String

    if (!memcmp (vendor_str(), "GenuineIntel", 12))
      cpu_vendor = VENDOR_INTEL;
    else if (!memcmp (vendor_str(), "AuthenticAMD", 12))
      cpu_vendor = VENDOR_AMD;
    else if (!memcmp (vendor_str(), "CyrixInstead", 12))
      cpu_vendor = VENDOR_CYRIX;
    else if (!memcmp (vendor_str(), "CentaurHauls", 12))
      cpu_vendor = VENDOR_IDT;
    else if (!memcmp (vendor_str(), "UMC UMC UMC ", 12))
      cpu_vendor = VENDOR_UMC;
    else if (!memcmp (vendor_str(), "NexGenDriven", 12))
      cpu_vendor = VENDOR_NEXGEN;
    else if (!memcmp (vendor_str(), "RiseRiseRise", 12))
      cpu_vendor = VENDOR_RISE;
    else if (!memcmp (vendor_str(), "GenuineTMx86", 12))
      cpu_vendor = VENDOR_TRANSMETA;

    switch (max) {		
      default:
	// All cases fall through!
      case 2:
        if (cpu_vendor == VENDOR_INTEL)
          cache_tlb_intel();
      case 1:
        cpuid (1, &cpu_version, &cpu_apic, &cpu_ext_features, &cpu_features);
    }

    // Get maximum number for extended functions
    cpuid (0x80000000, &max, &dummy, &dummy, &dummy);

    if (max > 0x80000000) {
      switch (max) {
        default:
  	// All cases fall through!
        case 0x80000006:
          if (cpu_vendor == VENDOR_AMD)
            cache_tlb_amd_l2();
        case 0x80000005:
          if (cpu_vendor == VENDOR_AMD)
            cache_tlb_amd_l1();
        case 0x80000004:
        case 0x80000003:
        case 0x80000002:
        case 0x80000001:
	  break;
      }
    }

    // see Intel Spec on SYSENTER:
    // Some Pentium Pro pretend to have it, but actually lack it
    if (cpu_features & FEAT_SEP)
      if ((cpu_version & 0xFFF) < 0x633)
        cpu_features &= ~FEAT_SEP;

  } else
    cpu_version = 0x400;

  set_model_str (cpu_vendor, cpu_version, cpu_l2_cache_size);

  set_eflags (eflags);
}

static inline
Unsigned32
Cpu::muldiv (Unsigned32 val, Unsigned32 mul, Unsigned32 div)
{
  Unsigned32 dummy;

  asm volatile ("mull %3 ; divl %4\n\t"
               :"=a" (val), "=d" (dummy)
               : "0" (val),  "d" (mul),  "c" (div));
  return val;
}

IMPLEMENT inline
Unsigned64
Cpu::ns_to_tsc (Unsigned64 ns)
{
  Unsigned32 dummy;
  Unsigned64 tsc;
  asm volatile
	("movl  %%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "movl	%%ecx, %%eax		\n\t"
	 "movl	%%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "addl	%%ecx, %%eax		\n\t"
	 "adcl	$0, %%edx		\n\t"
	 "shld	$5, %%eax, %%edx	\n\t"
	 "shll	$5, %%eax		\n\t"
	:"=A" (tsc), "=c" (dummy)
	: "0" (ns),  "b" (scaler_ns_to_tsc)
	);
  return tsc;
}

IMPLEMENT inline
Unsigned64
Cpu::tsc_to_ns (Unsigned64 tsc)
{
  Unsigned32 dummy;
  Unsigned64 ns;
  asm volatile
	("movl  %%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "movl	%%ecx, %%eax		\n\t"
	 "movl	%%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "addl	%%ecx, %%eax		\n\t"
	 "adcl	$0, %%edx		\n\t"
	 "shld	$5, %%eax, %%edx	\n\t"
	 "shll	$5, %%eax		\n\t"
	:"=A" (ns), "=c" (dummy)
	: "0" (tsc), "b" (scaler_tsc_to_ns)
	);
  return ns;
}

IMPLEMENT inline
Unsigned64
Cpu::tsc_to_us (Unsigned64 tsc)
{
  Unsigned32 dummy;
  Unsigned64 us;
  asm volatile
	("movl  %%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "movl	%%ecx, %%eax		\n\t"
	 "movl	%%edx, %%ecx		\n\t"
	 "mull	%3			\n\t"
	 "addl	%%ecx, %%eax		\n\t"
	 "adcl	$0, %%edx		\n\t"
	 "shld	$5, %%eax, %%edx	\n\t"
	 "shll	$5, %%eax		\n\t"
	:"=A" (us), "=c" (dummy)
	: "0" (tsc), "S" (scaler_tsc_to_us)
	);
  return us;
}

IMPLEMENT inline
void
Cpu::tsc_to_s_and_ns (Unsigned64 tsc, Unsigned32 *s, Unsigned32 *ns)
{
    Unsigned32 dummy;
    __asm__
	("				\n\t"
	 "movl  %%edx, %%ecx		\n\t"
	 "mull	%4			\n\t"
	 "movl	%%ecx, %%eax		\n\t"
	 "movl	%%edx, %%ecx		\n\t"
	 "mull	%4			\n\t"
	 "addl	%%ecx, %%eax		\n\t"
	 "adcl	$0, %%edx		\n\t"
	 "movl  $1000000000, %%ecx	\n\t"
	 "shld	$5, %%eax, %%edx	\n\t"
	 "shll	$5, %%eax		\n\t"
	 "divl  %%ecx			\n\t"
	:"=a" (*s), "=d" (*ns), "=c" (dummy)
	: "A" (tsc), "g" (scaler_tsc_to_ns)
	);
}

IMPLEMENT inline
Unsigned64
Cpu::rdtsc (void)
{
  Unsigned64 tsc;

  asm volatile ("rdtsc" : "=A" (tsc));
  return tsc;
}

IMPLEMENT inline
Unsigned32
Cpu::get_scaler_tsc_to_ns()
{
  return scaler_tsc_to_ns;
}

IMPLEMENT inline
Unsigned32
Cpu::get_scaler_tsc_to_us()
{
  return scaler_tsc_to_us;
}

IMPLEMENT inline
Unsigned32
Cpu::get_scaler_ns_to_tsc()
{
  return scaler_ns_to_tsc;
}

