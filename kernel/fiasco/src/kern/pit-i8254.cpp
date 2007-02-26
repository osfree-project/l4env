INTERFACE:
class Pit
{
public:
  static void init( unsigned rate );
  static void done();
  static void set_freq_slow();
  static void set_freq_normal();
  static void setup_channel2_to_200hz();
};

IMPLEMENTATION[i8254]:

#include "io.h"
#include "pic.h"
#include "initcalls.h"


// set up timer interrupt (~ 1ms)
IMPLEMENT inline NEEDS ["io.h","pic.h"]
void Pit::init( unsigned rate )
{
  // set counter channel 0 to binary, mode2, lsb/msb
  Io::out8_p(0x34, 0x43);

  // set counter frequency to ~1000 Hz (1000.151 Hz)
  unsigned t = 1193180 / rate;
  Io::out8_p(t & 0xff,0x40);
  Io::out8_p((t >> 8) & 0xff,0x40);
  
  // allow this interrupt
  Pic::enable(0);
}

IMPLEMENT inline
void Pit::done()
{
}

IMPLEMENT inline NEEDS ["io.h"]
void Pit::set_freq_slow()
{
  // set counter channel 0 to binary, mode2, lsb/msb
  Io::out8_p(0x34, 0x43);

  // set counter frequency to 20 Hz
  unsigned t = 59659;
  Io::out8_p(t & 0xff,0x40);
  Io::out8_p((t >> 8) & 0xff,0x40);
}

IMPLEMENT
inline NEEDS ["io.h"]
void Pit::set_freq_normal()
{
  // set counter channel 0 to binary, mode2, lsb/msb
  Io::out8_p(0x34,0x43);
  
  // set counter frequency to ~1000 Hz (1000.151 Hz)
  unsigned t = 1193180 / 1000;
  Io::out8_p(t & 0xff,0x40);
  Io::out8_p((t >> 8) & 0xff,0x40);
}

IMPLEMENT FIASCO_INIT
void
Pit::setup_channel2_to_200hz()
{
#define CLOCK_TICK_RATE		1193180	// i8254 ticks per second
#define CALIBRATE_TIME		500001			// 50ms
#define CALIBRATE_LATCH		(CLOCK_TICK_RATE / 20)	// 50ms
  // set gate high, disable speaker
  Io::out8((Io::in8(0x61) & ~0x02) | 0x01, 0x61);

  // set counter channel 2 to binary, mode0, lsb/msb
  Io::out8(0xb0, 0x43);
  // set counter frequency
  Io::out8(CALIBRATE_LATCH & 0xff, 0x42);
  Io::out8(CALIBRATE_LATCH >> 8,   0x42);
}

