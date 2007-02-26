INTERFACE:

#include "types.h"

#define FIASCO_FASTCALL	  __attribute__((regparm(3)))

class Perf_cnt
{
public:
  enum Unit_mask_type
    { None, Fixed, Exclusive, Bitmask, };

  typedef Unsigned64 FIASCO_FASTCALL (*Perf_read_fn)();
  static  Perf_read_fn read_pmc[2];
};


IMPLEMENTATION[disabled]:

Perf_cnt::Perf_read_fn Perf_cnt::read_pmc[2] =
{ dummy_read_pmc, dummy_read_pmc };

static FIASCO_FASTCALL
Unsigned64 dummy_read_pmc()
{
  return 0;
}

PUBLIC static void
Perf_cnt::get_unit_mask(Mword, Unit_mask_type *, Mword *, Mword *)
{
}

PUBLIC static void
Perf_cnt::get_unit_mask_entry(Mword, Mword, Mword *, const char **)
{
}

PUBLIC static void
Perf_cnt::get_perf_event(Mword, Mword *, const char **, const char **)
{
}

PUBLIC static Mword
Perf_cnt::get_max_perf_event()
{
  return 0;
}

PUBLIC static void
Perf_cnt::split_event(Mword, Mword *, Mword *)
{
}

PUBLIC static Mword
Perf_cnt::lookup_event(Mword)
{
  return 0;
}

PUBLIC static void
Perf_cnt::combine_event(Mword, Mword, Mword *)
{
}

PUBLIC static char const *
Perf_cnt::perf_type()
{
  return "nothing";
}

PUBLIC static int
Perf_cnt::perf_mode(Mword, const char **mode, const char **name,
		    Mword *event, Mword *user, Mword *kern, Mword *edge)
{
  *mode  = "";
  *name  = "";
  *event = 0;
  *user  = 0;
  *kern  = 0;
  *edge  = 0;
  return 0;
}

PUBLIC static void
Perf_cnt::setup_pmc(Mword, Mword, Mword, Mword, Mword)
{
}

