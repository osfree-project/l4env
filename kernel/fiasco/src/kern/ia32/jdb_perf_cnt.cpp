INTERFACE:

#include "types.h"

#define FIASCO_FASTCALL	__attribute__((regparm(3)))

class Jdb_perf_cnt
{
public:
  // init performance counter
  static void FIASCO_FASTCALL (*_init_pmc)(unsigned event, 
					   int user, int kernel);
  // read performance counter
  static Unsigned64 FIASCO_FASTCALL (*read_pmc)();

  static void init_pmc(unsigned event, int user, int kernel);
  static void init();
  static bool have_tsc();
  static char const *perf_type();
  static int perf_mode(const char **type, const char **mode, 
		       unsigned *event, int *user, int *kernel);

private:

  static int perf_user;
  static int perf_kernel;
  static unsigned perf_event;

  static bool _have_tsc;
  static char const *_perf_type;

};


IMPLEMENTATION:

#include "cpu.h"
#include "regdefs.h"

void (*Jdb_perf_cnt::_init_pmc)(unsigned event, int user, int kernel) 
  = &dummy_init_pmc;
Unsigned64 (*Jdb_perf_cnt::read_pmc)() = &dummy_read_pmc;

int Jdb_perf_cnt::perf_kernel;
int Jdb_perf_cnt::perf_user;
unsigned Jdb_perf_cnt::perf_event;

bool Jdb_perf_cnt::_have_tsc;
char const *Jdb_perf_cnt::_perf_type;

enum {
  // Intel P5
  MSR_P5_CESR           = 0x11,
  MSR_P5_CTR0           = 0x12,
  P5_EVNTSEL_USER       = 0x00000080,
  P5_EVNTSEL_KERNEL     = 0x00000040,
  
  // Intel P6
  MSR_P6_PERFCTR0       = 0xC1,
  MSR_P6_EVNTSEL0       = 0x186,
  P6_EVNTSEL_ENABLE     = 0x00400000,
  P6_EVNTSEL_USER       = 0x00010000,
  P6_EVNTSEL_KERNEL     = 0x00020000,
  
  // AMD K7
  MSR_K7_EVNTSEL0       = 0xC0010000,
  MSR_K7_PERFCTR0       = 0xC0010004,
  K7_EVNTSEL_ENABLE     = P6_EVNTSEL_ENABLE,
  K7_EVNTSEL_USER       = P6_EVNTSEL_USER,
  K7_EVNTSEL_KERNEL     = P6_EVNTSEL_KERNEL,
};

IMPLEMENT 
void Jdb_perf_cnt::init()
{
  _init_pmc = &dummy_init_pmc;
  read_pmc  = &dummy_read_pmc;
 
  _have_tsc = Cpu::features() & FEAT_TSC;
      
  if (_have_tsc && (Cpu::features() & FEAT_MSR))
    {
      if (Cpu::vendor() == Cpu::VENDOR_INTEL)
	{
	  // Intel
	  switch (Cpu::family())
	    {
	    case 5:
	      _init_pmc  = p5_init_pmc;
	      read_pmc   = p5_read_pmc;
	      _perf_type = "P5";
	      break;
	    case 6:
	      _init_pmc  = p6_init_pmc;
	      read_pmc   = p6_read_pmc;
	      _perf_type = "P6";
	      break;
	    case 15:
	      _init_pmc  = p4_init_pmc;
	      read_pmc   = p4_read_pmc;
	      _perf_type = "P4";
	      break;
	    }
	}
      else if (Cpu::vendor() == Cpu::VENDOR_AMD)
	{
	  // AMD
	  switch (Cpu::family())
	    {
	    case 6:
	      _init_pmc  = k7_init_pmc;
	      read_pmc   = k7_read_pmc;
	      _perf_type = "K7";
	      break;
	    }
	}
    }
}

IMPLEMENT inline 
bool Jdb_perf_cnt::have_tsc()
{
  return _have_tsc;
}


IMPLEMENT inline
char const *Jdb_perf_cnt::perf_type()
{
  return _perf_type;
}

// return type of performance registers we have
IMPLEMENT
int Jdb_perf_cnt::perf_mode(const char **type, const char **mode, 
			    unsigned *event, int *user, int *kernel)
{
  if ((*type = perf_type()))
    {
      if (!perf_kernel && !perf_user)
	*mode = "off";
      else
	{
	  if (perf_kernel && perf_user)
	    *mode = "K+U";
	  else if (perf_kernel)
	    *mode = "K";
	  else
	    *mode = "U";
	}
      
      *event  = perf_event;
      *user   = perf_user;
      *kernel = perf_kernel;
      return 1;
    }
  
  *type   = "n/a";
  *mode   = "";
  *event  = 0;
  *user   = 0;
  *kernel = 0;
  return 0;
}

IMPLEMENT inline
void Jdb_perf_cnt::init_pmc(unsigned event, int user, int kernel)
{
  _init_pmc( perf_event  = event,
	     perf_user   = user,
	     perf_kernel = kernel );
}

//--------------------------------------------------------------------
// dummy
static FIASCO_FASTCALL
void dummy_init_pmc(unsigned, int, int)
{}

static FIASCO_FASTCALL
Unsigned64 dummy_read_pmc()
{
  return 0;
}

// Intel P5
static FIASCO_FASTCALL
void p5_init_pmc(unsigned event, int user, int kernel)
{
  if (user)
    event += P5_EVNTSEL_USER;
  if (kernel)
    event += P5_EVNTSEL_KERNEL;
  
  // select event
  Cpu::wrmsr(event, MSR_P5_CESR);

  // reset performance counter 0
  Cpu::wrmsr(0, MSR_P5_CTR0);
}

// Intel P5
static FIASCO_FASTCALL
Unsigned64 p5_read_pmc()
{
  // read performance counter 0
  return Cpu::rdmsr(MSR_P5_CTR0);
}

// Intel P6
static FIASCO_FASTCALL
void p6_init_pmc(unsigned event, int user, int kernel)
{
  if (user)
    event += P6_EVNTSEL_USER;
  if (kernel)
    event += P6_EVNTSEL_KERNEL;
  
  // select event
  Cpu::wrmsr(event + P6_EVNTSEL_ENABLE, MSR_P6_EVNTSEL0);

  // reset performance counter 0
  Cpu::wrmsr(0, MSR_P6_PERFCTR0);
}

// Intel P6
static FIASCO_FASTCALL
Unsigned64 p6_read_pmc()
{
  // read performance counter 0
  return Cpu::rdpmc(0);
}

// Intel P4
static FIASCO_FASTCALL
void p4_init_pmc(unsigned /*event*/, int /*user*/, int /*kernel*/)
{
#if 0
  // XXX
  if (user)
    event += P6_EVNTSEL_USER;
  if (kernel)
    event += P6_EVNTSEL_KERNEL;
  
  // XXX
  // select event
  Cpu::wrmsr(event + P6_EVNTSEL_ENABLE, MSR_P6_EVNTSEL0);

  // XXX
  // reset performance counter 0
  Cpu::wrmsr(0, MSR_P6_PERFCTR0);
#endif
}

// Intel P4
static FIASCO_FASTCALL
Unsigned64 p4_read_pmc()
{
#if 0
  // XXX
  // read performance counter 0
  return Cpu::rdmsr(MSR_P6_PERFCTR0);
#else
  return 0;
#endif
}

// AMD K7
static FIASCO_FASTCALL
void k7_init_pmc(unsigned event, int user, int kernel)
{
  if (user)
    event += K7_EVNTSEL_USER;
  if (kernel)
    event += K7_EVNTSEL_KERNEL;
  
  // select event
  Cpu::wrmsr(event + K7_EVNTSEL_ENABLE, MSR_K7_EVNTSEL0);
  
  // reset counter 0
  Cpu::wrmsr(0, MSR_K7_PERFCTR0);
}

// AMD K7
static FIASCO_FASTCALL
Unsigned64 k7_read_pmc()
{
  // read performance counter 0
  return Cpu::rdmsr(MSR_K7_PERFCTR0);
}
