/*
 * Fiasco-UX
 * Architecture specific config code
 */

INTERFACE:

EXTENSION class Config
{
public:

  static const bool backward_compatibility = true;
  static const bool scheduling_using_pit = true;

  static const bool hlt_works_ok = true;
  static const bool getchar_does_hlt = false;
  static const bool pic_prio_modify = true;
  static const bool enable_io_protection = false;
  static const bool kinfo_timer_uses_rdtsc = false;

  static const unsigned microsec_per_tick = 10000LL;
};

IMPLEMENTATION[ux]:
//-
