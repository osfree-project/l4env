#include <l4/crtx/ctor.h>
#include <l4/log/l4log.h>

char LOG_tag[9]="initcall";

class Init5 { public: Init5(); ~Init5(); };
class Init6 { public: Init6(); ~Init6(); };
class Init7 { public: Init7(); ~Init7(); };
class Init8 { public: Init8(); ~Init8(); };

static void __attribute__((constructor)) init1() { LOG_Enter(); }
static void __attribute__((constructor)) init2() { LOG_Enter(); }
static void                              init3() { LOG_Enter(); }
static void                              init4() { LOG_Enter(); }
L4C_CTOR(init3, L4CTOR_AFTER_BACKEND);
L4C_CTOR(init4, L4CTOR_AFTER_BACKEND);
static void __attribute__((destructor))  done1() { LOG_Enter(); }
static void __attribute__((destructor))  done2() { LOG_Enter(); }
static void                              done3() { LOG_Enter(); }
static void                              done4() { LOG_Enter(); }
L4C_DTOR(done3, L4CTOR_AFTER_BACKEND);
L4C_DTOR(done4, L4CTOR_AFTER_BACKEND);
                                  Init5::Init5() { LOG_Enter(); }
                                 Init5::~Init5() { LOG_Enter(); }
                                  Init6::Init6() { LOG_Enter(); }
                                 Init6::~Init6() { LOG_Enter(); }
                                  Init7::Init7() { LOG_Enter(); }
                                 Init7::~Init7() { LOG_Enter(); }
                                  Init8::Init8() { LOG_Enter(); }
                                 Init8::~Init8() { LOG_Enter(); }

Init5 init5 __attribute__((init_priority(2000)));
Init6 init6 __attribute__((init_priority(1000)));
Init7 init7;
Init8 init8;

int main(int argc, char**argv)
{
  LOG("This is the main function...");
  return 0;
}
