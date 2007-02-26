INTERFACE:

#include "initcalls.h"

class Timer
{
public:
  /**
   * @brief Static constructor for the interval timer.
   *
   * The implementation is platform specific. Two x86 implementations
   * are timer-pit and timer-rtc.
   */
  static void init() FIASCO_INIT;

  /**
   * @brief Acknowledges a timer IRQ.
   *
   * The implementation is platform specific.
   */
  static void acknowledge();

  /**
   * @brief Enables the intervall timer IRQ.
   *
   * The implementation is platform specific.
   */
  static void enable();

  /**
   * @brief Disabled the timer IRQ.
   */
  static void disable();

  /**
   * @brief Advances the system clock and handles timeouts.
   */
  static void update_system_clock();
};

IMPLEMENTATION:
