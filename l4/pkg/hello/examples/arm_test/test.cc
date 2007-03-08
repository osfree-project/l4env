#include "test.h"
#include <l4/cxx/iostream.h>

Test *Test::tests[max_tests];

void Test::add_test( Test *t )
{
  for( unsigned x=0; x <max_tests; ++x)
    if(!tests[x]) {
      tests[x] = t;
      break;
    }
}


void Test::run_tests()
{
  L4::cout << "Running tests:\n";
  for( unsigned x=0; x <max_tests; ++x)
    if(tests[x]) 
      L4::cout << "    " << tests[x]->name() << ": "
               << (tests[x]->run() ? "ok" : "failed") 
               << "\n";
  L4::cout << "done.\n";
}
