INTERFACE[ia32,amd64,ux]:

#include "types.h"
#include "initcalls.h"

class Cpu
{
public:

  enum Vendor
  {
    Vendor_unknown = 0,
    Vendor_intel,
    Vendor_amd,
    Vendor_cyrix,
    Vendor_via,
    Vendor_umc,
    Vendor_nexgen,
    Vendor_rise,
    Vendor_transmeta,
    Vendor_sis,
    Vendor_nsc
  };

  enum CacheTLB
  {
    Cache_unknown = 0,
    Cache_l1_data,
    Cache_l1_inst,
    Cache_l1_trace,
    Cache_l2,
    Cache_l3,
    Tlb_data_4k,
    Tlb_inst_4k,
    Tlb_data_4M,
    Tlb_inst_4M,
    Tlb_data_4k_4M,
    Tlb_inst_4k_4M,
  };

  enum
  {
    Ldt_entry_size = 8,
  };

  enum Local_features
  {
    Lf_rdpmc		= 0x00000001,
    Lf_rdpmc32		= 0x00000002,
  };

  static void			init();
  static Unsigned64		time_us();
  static int			can_wrmsr();

private:

  struct Vendor_table {
    Unsigned32			vendor_mask;
    Unsigned32			vendor_code;
    Unsigned16			l2_cache;
    char			vendor_string[32];
  } __attribute__((packed));

  struct Cache_table {
    Unsigned8			desc;
    Unsigned8			level;
    Unsigned16			size;
    Unsigned8			asso;
    Unsigned8			line_size;
  };

  static Unsigned64		_frequency;
  static Unsigned32		_version;
  static Unsigned32		_brand;
  static Unsigned32		_features;
  static Unsigned32		_ext_features;
  static Unsigned32		_ext_amd_features;
  static Unsigned32		_local_features;

  static Unsigned16		_inst_tlb_4k_entries;
  static Unsigned16		_data_tlb_4k_entries;
  static Unsigned16		_inst_tlb_4m_entries;
  static Unsigned16		_data_tlb_4m_entries;
  static Unsigned16		_inst_tlb_4k_4m_entries;
  static Unsigned16		_data_tlb_4k_4m_entries;
  static Unsigned16		_l2_inst_tlb_4k_entries;
  static Unsigned16		_l2_data_tlb_4k_entries;
  static Unsigned16		_l2_inst_tlb_4m_entries;
  static Unsigned16		_l2_data_tlb_4m_entries;

  static Unsigned16		_l1_trace_cache_size;
  static Unsigned16		_l1_trace_cache_asso;

  static Unsigned16		_l1_data_cache_size;
  static Unsigned16		_l1_data_cache_asso;
  static Unsigned16		_l1_data_cache_line_size;

  static Unsigned16		_l1_inst_cache_size;
  static Unsigned16		_l1_inst_cache_asso;
  static Unsigned16		_l1_inst_cache_line_size;

  static Unsigned16		_l2_cache_size;
  static Unsigned16		_l2_cache_asso;
  static Unsigned16		_l2_cache_line_size;

  static Unsigned8		_phys_bits;
  static Unsigned8		_virt_bits;

  static Vendor			_vendor;
  static char			_model_str[32];

  static Vendor_table const	intel_table[];
  static Vendor_table const	amd_table[];
  static Vendor_table const	cyrix_table[];
  static Vendor_table const	via_table[];
  static Vendor_table const	umc_table[];
  static Vendor_table const	nexgen_table[];
  static Vendor_table const	rise_table[];
  static Vendor_table const	transmeta_table[];
  static Vendor_table const	sis_table[];
  static Vendor_table const	nsc_table[];

  static Cache_table const	intel_cache_table[];

  static char const * const vendor_ident[];
  static Vendor_table const * const	vendor_table[];

  static Unsigned32		scaler_tsc_to_ns;
  static Unsigned32		scaler_tsc_to_us;
  static Unsigned32		scaler_ns_to_tsc;

  static char const * const     exception_strings[];
};

IMPLEMENTATION[ia32,amd64,ux]:

#include <cstdio>
#include <cstring>
#include "config.h"
#include "panic.h"
#include "processor.h"
#include "regdefs.h"

Unsigned64	Cpu::_frequency;
Unsigned32	Cpu::_version;
Unsigned32	Cpu::_brand;
Unsigned32	Cpu::_features;
Unsigned32	Cpu::_ext_features;
Unsigned32	Cpu::_ext_amd_features;
Unsigned32	Cpu::_local_features;

Unsigned16	Cpu::_inst_tlb_4k_entries;
Unsigned16	Cpu::_data_tlb_4k_entries;
Unsigned16	Cpu::_inst_tlb_4m_entries;
Unsigned16	Cpu::_data_tlb_4m_entries;
Unsigned16	Cpu::_inst_tlb_4k_4m_entries;
Unsigned16	Cpu::_data_tlb_4k_4m_entries;
Unsigned16	Cpu::_l2_inst_tlb_4k_entries;
Unsigned16	Cpu::_l2_data_tlb_4k_entries;
Unsigned16	Cpu::_l2_inst_tlb_4m_entries;
Unsigned16	Cpu::_l2_data_tlb_4m_entries;

Unsigned16	Cpu::_l1_trace_cache_size;
Unsigned16	Cpu::_l1_trace_cache_asso;

Unsigned16	Cpu::_l1_data_cache_size;
Unsigned16	Cpu::_l1_data_cache_asso;
Unsigned16	Cpu::_l1_data_cache_line_size;

Unsigned16	Cpu::_l1_inst_cache_size;
Unsigned16	Cpu::_l1_inst_cache_asso;
Unsigned16	Cpu::_l1_inst_cache_line_size;

Unsigned16	Cpu::_l2_cache_size;
Unsigned16	Cpu::_l2_cache_asso;
Unsigned16	Cpu::_l2_cache_line_size;

Unsigned8	Cpu::_phys_bits = 32;
Unsigned8	Cpu::_virt_bits = 32;

Cpu::Vendor     Cpu::_vendor;
char		Cpu::_model_str[32];

Cpu::Vendor_table const Cpu::intel_table[] FIASCO_INITDATA =
{
  { 0xFF0, 0x400, 0xFFFF, "i486 DX-25/33"                   },
  { 0xFF0, 0x410, 0xFFFF, "i486 DX-50"                      },
  { 0xFF0, 0x420, 0xFFFF, "i486 SX"                         },
  { 0xFF0, 0x430, 0xFFFF, "i486 DX/2"                       },
  { 0xFF0, 0x440, 0xFFFF, "i486 SL"                         },
  { 0xFF0, 0x450, 0xFFFF, "i486 SX/2"                       },
  { 0xFF0, 0x470, 0xFFFF, "i486 DX/2-WB"                    },
  { 0xFF0, 0x480, 0xFFFF, "i486 DX/4"                       },
  { 0xFF0, 0x490, 0xFFFF, "i486 DX/4-WB"                    },
  { 0xFF0, 0x500, 0xFFFF, "Pentium A-Step"                  },
  { 0xFF0, 0x510, 0xFFFF, "Pentium P5"                      },
  { 0xFF0, 0x520, 0xFFFF, "Pentium P54C"                    },
  { 0xFF0, 0x530, 0xFFFF, "Pentium P24T Overdrive"          },
  { 0xFF0, 0x540, 0xFFFF, "Pentium P55C MMX"                },
  { 0xFF0, 0x570, 0xFFFF, "Pentium Mobile"                  },
  { 0xFF0, 0x580, 0xFFFF, "Pentium MMX Mobile (Tillamook)"  },
  { 0xFF0, 0x600, 0xFFFF, "Pentium-Pro A-Step"              },
  { 0xFF0, 0x610, 0xFFFF, "Pentium-Pro"                     },
  { 0xFF0, 0x630, 512,    "Pentium II (Klamath)"            },
  { 0xFF0, 0x640, 512,    "Pentium II (Deschutes)"          },
  { 0xFF0, 0x650, 1024,   "Pentium II (Drake)"              },
  { 0xFF0, 0x650, 512,    "Pentium II (Deschutes)"          },
  { 0xFF0, 0x650, 256,    "Pentium II Mobile (Dixon)"       },
  { 0xFF0, 0x650, 0,      "Celeron (Covington)"             },
  { 0xFF0, 0x660, 128,    "Celeron (Mendocino)"             },
  { 0xFF0, 0x670, 1024,   "Pentium III (Tanner)"            },
  { 0xFF0, 0x670, 512,    "Pentium III (Katmai)"            },
  { 0xFF0, 0x680, 256,    "Pentium III (Coppermine)"        },
  { 0xFF0, 0x680, 128,    "Celeron (Coppermine)"            },
  { 0xFF0, 0x690, 1024,   "Pentium-M (Banias)"              },
  { 0xFF0, 0x690, 512,    "Celeron-M (Banias)"              },
  { 0xFF0, 0x6a0, 1024,   "Pentium III (Cascades)"          },
  { 0xFF0, 0x6b0, 512,    "Pentium III-S"                   },
  { 0xFF0, 0x6b0, 256,    "Pentium III (Tualatin)"          },
  { 0xFF0, 0x6d0, 2048,   "Pentium-M (Dothan)"              },
  { 0xFF0, 0x6d0, 1024,   "Celeron-M (Dothan)"              },
  { 0xFF0, 0x6e0, 2048,   "Core (Yonah)"                    },
  { 0xFF0, 0x6f0, 2048,   "Core 2 (Merom)"                  },
  { 0xF00, 0x700, 0xFFFF, "Itanium (Merced)"                },
  { 0xFF0, 0xf00, 256,    "Pentium 4 (Willamette/Foster)"   },
  { 0xFF0, 0xf10, 256,    "Pentium 4 (Willamette/Foster)"   },
  { 0xFF0, 0xf10, 128,    "Celeron (Willamette)"            },
  { 0xFF0, 0xf20, 512,    "Pentium 4 (Northwood/Prestonia)" },
  { 0xFF0, 0xf20, 128,    "Celeron (Northwood)"             },
  { 0xFF0, 0xf30, 1024,   "Pentium 4E (Prescott/Nocona)"    },
  { 0xFF0, 0xf30, 256,    "Celeron D (Prescott)"            },
  { 0xFF4, 0xf40, 1024,   "Pentium 4E (Prescott/Nocona)"    },
  { 0xFF4, 0xf44, 1024,   "Pentium D (Smithfield)"          },
  { 0xFF0, 0xf40, 256,    "Celeron D (Prescott)"            },
  { 0xFF0, 0xf60, 2048,   "Pentium D (Cedarmill/Presler)"   },
  { 0xFF0, 0xf60, 512,    "Celeron D (Cedarmill)"           },
  { 0x0,   0x0,   0xFFFF, ""                                }
};

Cpu::Vendor_table const Cpu::amd_table[] FIASCO_INITDATA =
{
  { 0xFF0, 0x430, 0xFFFF, "Am486DX2-WT"                     },
  { 0xFF0, 0x470, 0xFFFF, "Am486DX2-WB"                     },
  { 0xFF0, 0x480, 0xFFFF, "Am486DX4-WT / Am5x86-WT"         },
  { 0xFF0, 0x490, 0xFFFF, "Am486DX4-WB / Am5x86-WB"         },
  { 0xFF0, 0x4a0, 0xFFFF, "SC400"                           },
  { 0xFF0, 0x4e0, 0xFFFF, "Am5x86-WT"                       },
  { 0xFF0, 0x4f0, 0xFFFF, "Am5x86-WB"                       },
  { 0xFF0, 0x500, 0xFFFF, "K5 (SSA/5) PR75/90/100"          },
  { 0xFF0, 0x510, 0xFFFF, "K5 (Godot) PR120/133"            },
  { 0xFF0, 0x520, 0xFFFF, "K5 (Godot) PR150/166"            },
  { 0xFF0, 0x530, 0xFFFF, "K5 (Godot) PR200"                },
  { 0xFF0, 0x560, 0xFFFF, "K6 (Little Foot)"                },
  { 0xFF0, 0x570, 0xFFFF, "K6 (Little Foot)"                },
  { 0xFF0, 0x580, 0xFFFF, "K6-II (Chomper)"                 },
  { 0xFF0, 0x590, 256,    "K6-III (Sharptooth)"             },
  { 0xFF0, 0x5c0, 128,    "K6-2+"                           },
  { 0xFF0, 0x5d0, 256,    "K6-3+"                           },
  { 0xFF0, 0x600, 0xFFFF, "Athlon K7 (Argon)"               },
  { 0xFF0, 0x610, 0xFFFF, "Athlon K7 (Pluto)"               },
  { 0xFF0, 0x620, 0xFFFF, "Athlon K75 (Orion)"              },
  { 0xFF0, 0x630, 64,     "Duron (Spitfire)"                },
  { 0xFF0, 0x640, 256,    "Athlon (Thunderbird)"            },
  { 0xFF0, 0x660, 256,    "Athlon (Palomino)"               },
  { 0xFF0, 0x660, 64,     "Duron (Morgan)"                  },
  { 0xFF0, 0x670, 64,     "Duron (Morgan)"                  },
  { 0xFF0, 0x680, 256,    "Athlon (Thoroughbred)"           },
  { 0xFF0, 0x680, 64,     "Duron (Applebred)"               },
  { 0xFF0, 0x6A0, 512,    "Athlon (Barton)"                 },
  { 0xFF0, 0x6A0, 256,    "Athlon (Thorton)"                },
  { 0xfff0ff0,  0x00f00,    0,      "Athlon 64 (Clawhammer)"    },
  { 0xfff0ff0,  0x00f10,    0,      "Opteron (Sledgehammer)"    },
  { 0xfff0ff0,  0x00f40,    0,      "Athlon 64 (Clawhammer)"    },
  { 0xfff0ff0,  0x00f50,    0,      "Opteron (Sledgehammer)"    },
  { 0xfff0ff0,  0x00fc0,    512,    "Athlon64 (Newcastle)"      },
  { 0xfff0ff0,  0x00fc0,    256,    "Sempron (Paris)"           },
  { 0xfff0ff0,  0x10f50,    0,      "Opteron (Sledgehammer)"    },
  { 0xfff0ff0,  0x10fc0,    0,      "Sempron (Oakville)"        },
  { 0xfff0ff0,  0x10ff0,    0,      "Athlon 64 (Winchester)"    },
  { 0xfff0ff0,  0x20f10,    0,      "Opteron (Jackhammer)"      },
  { 0xfff0ff0,  0x20f30,    0,      "Athlon 64 X2 (Toledo)"     },
  { 0xfff0ff0,  0x20f40,    0,      "Turion 64 (Lancaster)"     },
  { 0xfff0ff0,  0x20f50,    0,      "Opteron (Venus)"           },
  { 0xfff0ff0,  0x20f70,    0,      "Athlon 64 (San Diego)"     },
  { 0xfff0ff0,  0x20fb0,    0,      "Athlon 64 X2 (Manchester)" },
  { 0xfff0ff0,  0x20fc0,    0,      "Sempron (Palermo)"         },
  { 0xfff0ff0,  0x20ff0,    0,      "Athlon 64 (Venice)"        },
  { 0xfff0ff0,  0x40f30,    0,      "Athlon 64 X2 (Windsor)"    },
  { 0xfff0ff0,  0x40f80,    0,      "Turion 64 X2 (Taylor)"     },
  { 0x0,        0x0,        0,      ""                          }
};

Cpu::Vendor_table const Cpu::cyrix_table[] FIASCO_INITDATA =
{
  { 0xFF0, 0x440, 0xFFFF, "Gx86 (Media GX)"                 },
  { 0xFF0, 0x490, 0xFFFF, "5x86"                            },
  { 0xFF0, 0x520, 0xFFFF, "6x86 (M1)"                       },
  { 0xFF0, 0x540, 0xFFFF, "GXm"                             },
  { 0xFF0, 0x600, 0xFFFF, "6x86MX (M2)"                     },
  { 0x0,   0x0,   0xFFFF, ""                                }
};

Cpu::Vendor_table const Cpu::via_table[] FIASCO_INITDATA =
{
  { 0xFF0, 0x540, 0xFFFF, "IDT Winchip C6"                  },
  { 0xFF0, 0x580, 0xFFFF, "IDT Winchip 2A/B"                },
  { 0xFF0, 0x590, 0xFFFF, "IDT Winchip 3"                   },
  { 0xFF0, 0x650, 0xFFFF, "Via Jalapeno (Joshua)"           },
  { 0xFF0, 0x660, 0xFFFF, "Via C5A (Samuel)"                },
  { 0xFF8, 0x670, 0xFFFF, "Via C5B (Samuel 2)"              },
  { 0xFF8, 0x678, 0xFFFF, "Via C5C (Ezra)"                  },
  { 0xFF0, 0x680, 0xFFFF, "Via C5N (Ezra-T)"                },
  { 0xFF0, 0x690, 0xFFFF, "Via C5P (Nehemiah)"              },
  { 0xFF0, 0x6a0, 0xFFFF, "Via C5J (Esther)"                },
  { 0x0,   0x0,   0xFFFF, ""                                }
};

Cpu::Vendor_table const Cpu::umc_table[] FIASCO_INITDATA =
{
  { 0xFF0, 0x410, 0xFFFF, "U5D"                             },
  { 0xFF0, 0x420, 0xFFFF, "U5S"                             },
  { 0x0,   0x0,   0xFFFF, ""                                }
};

Cpu::Vendor_table const Cpu::nexgen_table[] FIASCO_INITDATA =
{
  { 0xFF0, 0x500, 0xFFFF, "Nx586"                           },
  { 0x0,   0x0,   0xFFFF, ""                                }
};

Cpu::Vendor_table const Cpu::rise_table[] FIASCO_INITDATA =
{
  { 0xFF0, 0x500, 0xFFFF, "mP6 (iDragon)"                   },
  { 0xFF0, 0x520, 0xFFFF, "mP6 (iDragon)"                   },
  { 0xFF0, 0x580, 0xFFFF, "mP6 (iDragon II)"                },
  { 0xFF0, 0x590, 0xFFFF, "mP6 (iDragon II)"                },
  { 0x0,   0x0,   0xFFFF, ""                                }
};

Cpu::Vendor_table const Cpu::transmeta_table[] FIASCO_INITDATA =
{
  { 0xFFF, 0x542, 0xFFFF, "TM3x00 (Crusoe)"                 },
  { 0xFFF, 0x543, 0xFFFF, "TM5x00 (Crusoe)"                 },
  { 0xFF0, 0xf20, 0xFFFF, "TM8x00 (Efficeon)"               },
  { 0x0,   0x0,   0xFFFF, ""                                }
};

Cpu::Vendor_table const Cpu::sis_table[] FIASCO_INITDATA =
{
  { 0xFF0, 0x500, 0xFFFF, "55x"                             },
  { 0x0,   0x0,   0xFFFF, ""                                }
};

Cpu::Vendor_table const Cpu::nsc_table[] FIASCO_INITDATA =
{
  { 0xFF0, 0x540, 0xFFFF, "Geode GX1"                       },
  { 0xFF0, 0x550, 0xFFFF, "Geode GX2"                       },
  { 0xFF0, 0x680, 0xFFFF, "Geode NX"                        },
  { 0x0,   0x0,   0xFFFF, ""                                }
};

Cpu::Cache_table const Cpu::intel_cache_table[] FIASCO_INITDATA =
{
  { 0x01, Tlb_inst_4k,        32,   4,    0 },
  { 0x02, Tlb_inst_4M,         2,   4,    0 },
  { 0x03, Tlb_data_4k,        64,   4,    0 },
  { 0x04, Tlb_data_4M,         8,   4,    0 },
  { 0x05, Tlb_data_4M,        32,   4,    0 },
  { 0x06, Cache_l1_inst,       8,   4,   32 },
  { 0x08, Cache_l1_inst,      16,   4,   32 },
  { 0x0A, Cache_l1_data,       8,   2,   32 },
  { 0x0B, Tlb_inst_4M,         4,   4,    0 },
  { 0x0C, Cache_l1_data,      16,   4,   32 },
  { 0x22, Cache_l3,          512,   4,   64 },  /* sectored */
  { 0x23, Cache_l3,         1024,   8,   64 },  /* sectored */
  { 0x25, Cache_l3,         2048,   8,   64 },  /* sectored */
  { 0x29, Cache_l3,         4096,   8,   64 },  /* sectored */
  { 0x2C, Cache_l1_data,      32,   8,   64 },
  { 0x30, Cache_l1_inst,      32,   8,   64 },
  { 0x39, Cache_l2,          128,   4,   64 },  /* sectored */
  { 0x3B, Cache_l2,          128,   2,   64 },  /* sectored */
  { 0x3C, Cache_l2,          256,   4,   64 },  /* sectored */
  { 0x41, Cache_l2,          128,   4,   32 },
  { 0x42, Cache_l2,          256,   4,   32 },
  { 0x43, Cache_l2,          512,   4,   32 },
  { 0x44, Cache_l2,         1024,   4,   32 },
  { 0x45, Cache_l2,         2048,   4,   32 },
  { 0x46, Cache_l3,         4096,   4,   64 },
  { 0x47, Cache_l3,         8192,   8,   64 },
  { 0x49, Cache_l2,         4096,  16,   64 },
  { 0x50, Tlb_inst_4k_4M,     64,   0,    0 },
  { 0x51, Tlb_inst_4k_4M,    128,   0,    0 },
  { 0x52, Tlb_inst_4k_4M,    256,   0,    0 },
  { 0x56, Tlb_data_4M,        16,   4,    0 },
  { 0x57, Tlb_data_4k,        16,   4,    0 },
  { 0x5B, Tlb_data_4k_4M,     64,   0,    0 },
  { 0x5C, Tlb_data_4k_4M,    128,   0,    0 },
  { 0x5D, Tlb_data_4k_4M,    256,   0,    0 },
  { 0x60, Cache_l1_data,      16,   8,   64 },
  { 0x66, Cache_l1_data,       8,   4,   64 },  /* sectored */
  { 0x67, Cache_l1_data,      16,   4,   64 },  /* sectored */
  { 0x68, Cache_l1_data,      32,   4,   64 },  /* sectored */
  { 0x70, Cache_l1_trace,     12,   8,    0 },
  { 0x71, Cache_l1_trace,     16,   8,    0 },
  { 0x72, Cache_l1_trace,     32,   8,    0 },
  { 0x77, Cache_l1_inst,      16,   4,   64 },
  { 0x78, Cache_l2,         1024,   4,   64 },
  { 0x79, Cache_l2,          128,   8,   64 },  /* sectored */
  { 0x7A, Cache_l2,          256,   8,   64 },  /* sectored */
  { 0x7B, Cache_l2,          512,   8,   64 },  /* sectored */
  { 0x7C, Cache_l2,         1024,   8,   64 },  /* sectored */
  { 0x7D, Cache_l2,         2048,   8,   64 },
  { 0x7E, Cache_l2,          256,   8,  128 },
  { 0x7F, Cache_l2,          512,   2,   64 },
  { 0x82, Cache_l2,          256,   8,   32 },
  { 0x83, Cache_l2,          512,   8,   32 },
  { 0x84, Cache_l2,         1024,   8,   32 },
  { 0x85, Cache_l2,         2048,   8,   32 },
  { 0x86, Cache_l2,          512,   4,   64 },
  { 0x87, Cache_l2,         1024,   8,   64 },
  { 0x8D, Cache_l3,         3072,  12,  128 },
  { 0xB0, Tlb_inst_4k,       128,   4,    0 },
  { 0xB3, Tlb_data_4k,       128,   4,    0 },
  { 0xB4, Tlb_data_4k,       256,   4,    0 },
  { 0x0,  Cache_unknown,       0,   0,    0 }
};

char const * const Cpu::vendor_ident[] =
{
   0,
  "GenuineIntel",
  "AuthenticAMD",
  "CyrixInstead",
  "CentaurHauls",
  "UMC UMC UMC ",
  "NexGenDriven",
  "RiseRiseRise",
  "GenuineTMx86",
  "SiS SiS SiS ",
  "Geode by NSC"
};

Cpu::Vendor_table const * const Cpu::vendor_table[] =
{
  0,
  intel_table,
  amd_table,
  cyrix_table,
  via_table,
  umc_table,
  nexgen_table,
  rise_table,
  transmeta_table,
  sis_table,
  nsc_table
};

Unsigned32	Cpu::scaler_tsc_to_ns;
Unsigned32	Cpu::scaler_tsc_to_us;
Unsigned32	Cpu::scaler_ns_to_tsc;

char const * const Cpu::exception_strings[] =
{
  /*  0 */ "Divide Error",
  /*  1 */ "Debug",
  /*  2 */ "NMI Interrupt",
  /*  3 */ "Breakpoint",
  /*  4 */ "Overflow",
  /*  5 */ "BOUND Range Exceeded",
  /*  6 */ "Invalid Opcode",
  /*  7 */ "Device Not Available",
  /*  8 */ "Double Fault",
  /*  9 */ "CoProcessor Segment Overrrun",
  /* 10 */ "Invalid TSS",
  /* 11 */ "Segment Not Present",
  /* 12 */ "Stack Segment Fault",
  /* 13 */ "General Protection",
  /* 14 */ "Page Fault",
  /* 15 */ "Reserved",
  /* 16 */ "Floating-Point Error",
  /* 17 */ "Alignment Check",
  /* 18 */ "Machine Check",
  /* 19 */ "SIMD Floating-Point Exception",
  /* 20 */ "Reserved",
  /* 21 */ "Reserved",
  /* 22 */ "Reserved",
  /* 23 */ "Reserved",
  /* 24 */ "Reserved",
  /* 25 */ "Reserved",
  /* 26 */ "Reserved",
  /* 27 */ "Reserved",
  /* 28 */ "Reserved",
  /* 29 */ "Reserved",
  /* 30 */ "Reserved",
  /* 31 */ "Reserved"
};

PUBLIC static 
char const *
Cpu::exception_string(Mword trapno) 
{
  if (trapno > 32)
    return "Maskable Interrupt";
  return exception_strings[trapno];
}

PRIVATE static inline FIASCO_INIT
void
Cpu::cpuid (Unsigned32 const mode,
            Unsigned32 *const eax, Unsigned32 *const ebx,
	    Unsigned32 *const ecx, Unsigned32 *const edx)
{
  asm volatile ("cpuid" : "=a" (*eax), "=b" (*ebx), "=c" (*ecx), "=d" (*edx)
                        : "a" (mode));
}

PUBLIC static inline
char const *
Cpu::vendor_str()
{
  return _vendor == Vendor_unknown ? "Unknown" : vendor_ident[_vendor];
}

PUBLIC static inline
char const*
Cpu::model_str()
{ return _model_str; }

PUBLIC static inline
Cpu::Vendor
Cpu::vendor()
{ return _vendor; }

PUBLIC static inline
unsigned int
Cpu::family()
{
  return (_version >> 8 & 0xf) + (_version >> 20 & 0xff);
}

PUBLIC static inline
unsigned int
Cpu::model()
{
  return (_version >> 4 & 0xf) + (_version >> 12 & 0xf0);
}

PUBLIC static inline
unsigned int
Cpu::stepping()
{ return _version & 0xF; }

PUBLIC static inline
unsigned int
Cpu::type()
{ return (_version >> 12) & 0x3; }

PUBLIC static inline
Unsigned64
Cpu::frequency()
{ return _frequency; }

PUBLIC static inline
unsigned int
Cpu::brand()
{ return _brand & 0xFF; }

PUBLIC static inline
unsigned int
Cpu::features()
{ return _features; }

PUBLIC static inline
unsigned int
Cpu::ext_features()
{ return _ext_features; }

PUBLIC static inline
unsigned int
Cpu::ext_amd_features()
{ return _ext_amd_features; }

PUBLIC static inline
unsigned int
Cpu::local_features()
{ return _local_features; }

PUBLIC static inline NEEDS ["regdefs.h"]
bool
Cpu::have_superpages()
{ return features() & FEAT_PSE; }

PUBLIC static inline NEEDS ["regdefs.h"]
bool
Cpu::have_tsc()
{ return features() & FEAT_TSC; }

PUBLIC static inline NEEDS ["regdefs.h"]
bool
Cpu::have_sysenter()
{ return features() & FEAT_SEP; }

PUBLIC static inline NEEDS ["regdefs.h"]
bool
Cpu::have_syscall()
{ return ext_amd_features() & FEATA_SYSCALL; }

PUBLIC static inline
Mword
Cpu::virt_bits()
{ return _virt_bits; }

PUBLIC static inline
Mword
Cpu::phys_bits()
{ return _phys_bits; }

PRIVATE static FIASCO_INIT
void
Cpu::cache_tlb_intel()
{
  Unsigned8 desc[16];
  unsigned i, count = 0;
  Cache_table const *table;

  do {

    cpuid (2, (Unsigned32 *)(desc),
              (Unsigned32 *)(desc + 4),
              (Unsigned32 *)(desc + 8),
              (Unsigned32 *)(desc + 12));

    for (i = 1; i < 16; i++) {

      // Null descriptor or register bit31 set (reserved)
      if (!desc[i] || (desc[i / 4 * 4 + 3] & (1 << 7)))	
        continue;

      for (table = intel_cache_table; table->desc; table++) {
        if (table->desc == desc[i]) {
          switch (table->level) {
            case Cache_l1_data:
              _l1_data_cache_size      = table->size;
              _l1_data_cache_asso      = table->asso;
              _l1_data_cache_line_size = table->line_size;
              break;
            case Cache_l1_inst:
              _l1_inst_cache_size      = table->size;
              _l1_inst_cache_asso      = table->asso;
              _l1_inst_cache_line_size = table->line_size;
              break;
            case Cache_l1_trace:
              _l1_trace_cache_size = table->size;
              _l1_trace_cache_asso = table->asso;
              break;
            case Cache_l2:
              _l2_cache_size      = table->size;
              _l2_cache_asso      = table->asso;
              _l2_cache_line_size = table->line_size;
              break;
            case Tlb_inst_4k:
              _inst_tlb_4k_entries += table->size;
              break;
            case Tlb_data_4k:
              _data_tlb_4k_entries += table->size;
              break;
	    case Tlb_inst_4M:
	      _inst_tlb_4m_entries += table->size;
	      break;
	    case Tlb_data_4M:
	      _data_tlb_4m_entries += table->size;
	      break;
	    case Tlb_inst_4k_4M:
	      _inst_tlb_4k_4m_entries += table->size;
	      break;
	    case Tlb_data_4k_4M:
	      _data_tlb_4k_4m_entries += table->size;
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

PRIVATE static FIASCO_INIT
void
Cpu::cache_tlb_l1()
{
  Unsigned32 eax, ebx, ecx, edx;
  cpuid (0x80000005, &eax, &ebx, &ecx, &edx);

  _l1_data_cache_size      = (ecx >> 24) & 0xFF;
  _l1_data_cache_asso      = (ecx >> 16) & 0xFF;
  _l1_data_cache_line_size =  ecx        & 0xFF;

  _l1_inst_cache_size      = (edx >> 24) & 0xFF;
  _l1_inst_cache_asso      = (edx >> 16) & 0xFF;
  _l1_inst_cache_line_size =  edx        & 0xFF;

  _data_tlb_4k_entries     = (ebx >> 16) & 0xFF;
  _inst_tlb_4k_entries     =  ebx        & 0xFF;
  _data_tlb_4m_entries     = (eax >> 16) & 0xFF;
  _inst_tlb_4m_entries     =  eax        & 0xFF;
}

PRIVATE static FIASCO_INIT
void
Cpu::cache_tlb_l2 (Cpu::Vendor vendor)
{
  Unsigned32 eax, ebx, ecx, edx;
  cpuid (0x80000006, &eax, &ebx, &ecx, &edx);

  if (vendor == Vendor_via)
    {
      _l2_cache_size          = (ecx >> 24) & 0xFF;
      _l2_cache_asso          = (ecx >> 16) & 0xFF;
    }
  else
    {
      _l2_data_tlb_4m_entries = (eax >> 16) & 0xFFF;
      _l2_inst_tlb_4m_entries =  eax        & 0xFFF;
      _l2_data_tlb_4k_entries = (ebx >> 16) & 0xFFF;
      _l2_inst_tlb_4k_entries =  ebx        & 0xFFF;
      _l2_cache_size          = (ecx >> 16) & 0xFFFF;
      _l2_cache_asso          = (ecx >> 12) & 0xF;
    }

  _l2_cache_line_size = ecx & 0xFF;
}

PRIVATE static FIASCO_INIT
void
Cpu::addr_size_info ()
{
  Unsigned32 eax, ebx, ecx, edx;
  cpuid (0x80000008, &eax, &ebx, &ecx, &edx);

  _phys_bits = eax & 0xff;
  _virt_bits = (eax & 0xff00) >> 8;
}

PRIVATE static FIASCO_INIT
void
Cpu::set_model_str (Cpu::Vendor const vendor, unsigned const version,
                    Unsigned16 const l2_cache)
{
  Vendor_table const *table;

  for (table = vendor_table[vendor]; table && table->vendor_mask; table++)
    if ((version & table->vendor_mask) == table->vendor_code &&
        (table->l2_cache == 0xFFFF || l2_cache >= table->l2_cache))
      {
	snprintf (_model_str, sizeof (_model_str), "%s",
		  table->vendor_string);
	return;
      }

  snprintf (_model_str, sizeof (_model_str), "Unknown CPU");
}

/** Identify the CPU features.
    Attention: This function may be called more than once. The reason is
    that enabling a Local APIC that was previously disabled by the BIOS
    may change the processor features. Therefore, this function has to
    be called again after the Local APIC was enabled.
 */
PUBLIC static FIASCO_INIT
void
Cpu::identify()
{
  Unsigned32 eflags = get_flags();

  // Reset members in case we get called more than once
  _inst_tlb_4k_entries    =
  _data_tlb_4k_entries    =
  _inst_tlb_4m_entries    =
  _data_tlb_4m_entries    =
  _inst_tlb_4k_4m_entries =
  _data_tlb_4k_4m_entries = 0;

  // Check for Alignment Check Support
  set_flags (eflags ^ EFLAGS_AC);
  if (((get_flags() ^ eflags) & EFLAGS_AC) == 0)
    panic ("CPU too old");

  // Check for CPUID Support
  set_flags (eflags ^ EFLAGS_ID);
  if ((get_flags() ^ eflags) & EFLAGS_ID) {

    Unsigned32 max, i;
    char vendor_id[12];

    cpuid (0, &max, (Unsigned32 *)(vendor_id),
                    (Unsigned32 *)(vendor_id + 8),
                    (Unsigned32 *)(vendor_id + 4));

    for (i = sizeof (vendor_ident) / sizeof (*vendor_ident) - 1; i; i--)
      if (!memcmp (vendor_id, vendor_ident[i], 12))
        break;

    _vendor = (Cpu::Vendor) i;

    switch (max)
      {
      default:
	// All cases fall through!
      case 2:
        if (_vendor == Vendor_intel)
          cache_tlb_intel();
      case 1:
        cpuid (1, &_version, &_brand, &_ext_features, &_features);
      }

    if (_vendor == Vendor_intel)
      {
	switch (family())
	  {
	  case 5:
	    // Avoid Pentium Erratum 74
	    if ((_features & FEAT_MMX) &&
		(model() != 4 ||
		 (stepping() != 4 && (stepping() != 3 || type() != 1))))
	      _local_features |= Lf_rdpmc;
	    break;
	  case 6:
	    // Avoid Pentium Pro Erratum 26
	    if (model() >= 3 || stepping() > 9)
	      _local_features |= Lf_rdpmc;
	    break;
	  case 15:
	    _local_features |= Lf_rdpmc;
	    _local_features |= Lf_rdpmc32;
	    break;
	  }
      }
    else if (_vendor == Vendor_amd)
      {
	switch (family())
	  {
	  case 6:
	  case 15:
	    _local_features |= Lf_rdpmc;
	    break;
	  }
      }

    // Get maximum number for extended functions
    cpuid (0x80000000, &max, &i, &i, &i);

    if (max > 0x80000000)
      {
	switch (max)
	  {
	  default:
	    // All cases fall through!
	  case 0x80000008:
	    if (_vendor == Vendor_amd || _vendor == Vendor_intel)
	      addr_size_info ();
	  case 0x80000007:
	  case 0x80000006:
	    if (_vendor == Vendor_amd || _vendor == Vendor_via)
	      cache_tlb_l2 (_vendor);
	  case 0x80000005:
	    if (_vendor == Vendor_amd || _vendor == Vendor_via)
	      cache_tlb_l1();
	  case 0x80000004:
	  case 0x80000003:
	  case 0x80000002:
	  case 0x80000001:
	    if (_vendor == Vendor_intel || _vendor == Vendor_amd)
      	      cpuid (0x80000001, &i, &i, &i, &_ext_amd_features);
	    break;
	  }
      }

    // see Intel Spec on SYSENTER:
    // Some Pentium Pro pretend to have it, but actually lack it
    if ((_version & 0xFFF) < 0x633)
      _features &= ~FEAT_SEP;

  } else
    _version = 0x400;

  set_model_str (_vendor, _version, _l2_cache_size);

  set_flags (eflags);
}

PUBLIC static inline NEEDS["processor.h"]
void
Cpu::busy_wait_ns (Unsigned64 ns)
{
  Unsigned64 stop = rdtsc () + ns_to_tsc(ns);

  while (rdtsc() < stop)
    Proc::pause();
}

PUBLIC static inline
Unsigned32
Cpu::get_scaler_tsc_to_ns()
{ return scaler_tsc_to_ns; }

PUBLIC static inline
Unsigned32
Cpu::get_scaler_tsc_to_us()
{ return scaler_tsc_to_us; }

PUBLIC static inline
Unsigned32
Cpu::get_scaler_ns_to_tsc()
{ return scaler_ns_to_tsc; }

PUBLIC static
void
Cpu::show_cache_tlb_info(const char *indent)
{
  char s[16];

  *s = '\0';
  if (_l2_inst_tlb_4k_entries)
    snprintf(s, sizeof(s), "/%u", _l2_inst_tlb_4k_entries);
  if (_inst_tlb_4k_entries)
    printf ("%s%4u%s Entry I TLB (4K pages)", indent, _inst_tlb_4k_entries, s);
  *s = '\0';
  if (_l2_inst_tlb_4m_entries)
    snprintf(s, sizeof(s), "/%u", _l2_inst_tlb_4k_entries);
  if (_inst_tlb_4m_entries)
    printf ("   %4u%s Entry I TLB (4M pages)", _inst_tlb_4m_entries, s);
  if (_inst_tlb_4k_4m_entries)
    printf ("%s%4u Entry I TLB (4K or 4M pages)",
	indent, _inst_tlb_4k_4m_entries);
  if (_inst_tlb_4k_entries || _inst_tlb_4m_entries || _inst_tlb_4k_4m_entries)
    putchar('\n');
  *s = '\0';
  if (_l2_data_tlb_4k_entries)
    snprintf(s, sizeof(s), "/%u", _l2_data_tlb_4k_entries);
  if (_data_tlb_4k_entries)
    printf ("%s%4u%s Entry D TLB (4K pages)", indent, _data_tlb_4k_entries, s);
  *s = '\0';
  if (_l2_data_tlb_4m_entries)
    snprintf(s, sizeof(s), "/%u", _l2_data_tlb_4m_entries);
  if (_data_tlb_4m_entries)
    printf ("   %4u%s Entry D TLB (4M pages)", _data_tlb_4m_entries, s);
  if (_data_tlb_4k_4m_entries)
    printf ("%s%4u Entry D TLB (4k or 4M pages)",
	indent, _data_tlb_4k_4m_entries);
  if (_data_tlb_4k_entries || _data_tlb_4m_entries || _data_tlb_4k_4m_entries)
    putchar('\n');

  if (_l1_trace_cache_size)
    printf ("%s%3uK %c-ops T Cache (%u-way associative)\n",
	     indent, _l1_trace_cache_size, Config::char_micro,
	     _l1_trace_cache_asso);

  else if (_l1_inst_cache_size)
    printf ("%s%4u KB L1 I Cache (%u-way associative, %u bytes per line)\n",
	     indent, _l1_inst_cache_size, _l1_inst_cache_asso,
	     _l1_inst_cache_line_size);

  if (_l1_data_cache_size)
    printf ("%s%4u KB L1 D Cache (%u-way associative, %u bytes per line)\n"
            "%s%4u KB L2 U Cache (%u-way associative, %u bytes per line)\n",
	     indent, _l1_data_cache_size, _l1_data_cache_asso,
	     _l1_data_cache_line_size,
	     indent, _l2_cache_size, _l2_cache_asso, _l2_cache_line_size);
}
