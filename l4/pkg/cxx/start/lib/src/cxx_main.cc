#include <l4/cxx/main_thread.h>
#include <l4/crtx/crt0.h>
#include <l4/util/util.h>

#include "cxx_atexit.h"

extern L4::MainThread *main;

extern "C" 
void __main()
{
  crt0_construction();
  //  __cxa_atexit( crt0_destruction );
  if(main)
    main->run();
  __cxa_do_atexit();
  crt0_sys_destruction();
  l4_sleep_forever();
}
