INTERFACE:

#include "initcalls.h"
#include "l4_types.h"

class Timer
{
public:
  /**
   * Static constructor for the interval timer.
   *
   * The implementation is platform specific. Two x86 implementations
   * are timer-pit and timer-rtc.
   */
  static void init() FIASCO_INIT;

  /**
   * Acknowledges a timer IRQ.
   *
   * The implementation is platform specific.
   */
  static void acknowledge();

  /**
   * Enables the intervall timer IRQ.
   *
   * The implementation is platform specific.
   */
  static void enable();

  /**
   * Disabled the timer IRQ.
   */
  static void disable();

  /**
   * Initialize the system clock.
   */
  static void init_system_clock();

  /**
   * Advances the system clock.
   */
  static void update_system_clock();

  /**
   * Get the current system clock.
   */
  static Unsigned64 system_clock();

  /**
   * reprogram the one-shot timer to the next event.
   */
  static void update_timer(Unsigned64 wakeup);
};

IMPLEMENTATION:
