INTERFACE:

#include "types.h"

/** 
 * Global CPU lock. When held, irqs are disabled on the current CPU 
 * (preventing nested irq handling, and preventing the current 
 * thread from being preempted).  It must only be held for very short
 * amounts of time.
 *
 * A generic (cli, sti) implementation of the lock can be found in 
 * cpu_lock-generic.cpp.
 */
class Cpu_lock
{
public:
  /// The return type of test methods
  typedef Mword Status;

  /// ctor.
  inline Cpu_lock();

  /**
   * @brief Test if the lock is already held.
   * @return 0 if the lock is not held, not 0 if it already is held.
   */
  Status test() const;

  /**
   * @brief Acquire the CPU lock.
   * The CPU lock disables IRQ's it should be held only for a very 
   * short amount of time.
   */
  void lock();

  /**
   * @brief Release the CPU lock.
   */
  void clear();

  /**
   * @brief Acquire the CPU lock and return the old status.
   * @return something else that 0 if the lock was already held and
   *   0 if it was not held. 
   */
  Status test_and_set();

  /**
   * @brief Set the CPU lock according to the given status.
   * @param state the state to set (0 clear, else lock).
   */
  void set(Status state);

private:
  /// Default copy constructor not implemented.
  Cpu_lock (const Cpu_lock&);

};

/**
 * The global CPU lock, contains the locking data necessary for some
 * special implementations.
 */
extern Cpu_lock cpu_lock;

IMPLEMENTATION:

#include "static_init.h"

Cpu_lock cpu_lock INIT_PRIORITY( MAX_INIT_PRIO );


IMPLEMENT inline //NEEDS [Cpu_lock::lock, Cpu_lock::test]
Cpu_lock::Status Cpu_lock::test_and_set()
{
  Status ret = test();
  lock();
  return ret;
}



IMPLEMENT inline //NEEDS [Cpu_lock::lock, Cpu_lock::clear]
void Cpu_lock::set(Cpu_lock::Status state)
{
  if (state)
    lock();
  else
    clear();
}


