#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/util/util.h>
#include <l4/util/reboot.h>
#include "test-client.h"
#include <stdlib.h> // needed for malloc
#include <stdio.h>

l4_ssize_t l4libc_heapsize = 500*1024;

const char *str1 = "Hallo";
const char *str2 = "Du!";
const char *str3 = "Dies ist ein etwas längerer String, der eigentlich schön lnag sein sollte!";
const char *str4 = "Dies ist ein etwas längerer String, der eigentlich schön lnag sein sollte! und damit er das auch ist, wird hier noch etwas drangehängt";
const char *str5 = "ene mene muh, dies ist ein etwas längerer String, der eigentlich schön lnag sein sollte!";

int main(int argc, char **argv) 
{
  int ret;
  struct test_dirent dirp;
  unsigned int count;
  l4_threadid_t srv;
  char *outstr = 0;
  CORBA_Environment env = dice_default_environment;
  env.malloc = (dice_malloc_func)malloc;
  
  LOG("Client(main): ask for name \"dcetst\"");
  while (names_waitfor_name("dcetst", &srv, 1000) == 0) {
      LOG("Client(main): \"dcetst\" not available, keep trying...");
  }
  LOG("Client(main): found \"dcetst\" at Names.");
  
  LOG("*** TEST 1 ***");
  LOG("Client(main): call test_ein_string with str1='%s'",str1);
  test_ein_string_call(&srv,str1,&env);
  
  LOG("*** TEST 2 ***");
  LOG("Client(main): call test_zwei_string with str1='%s', str2='%s'",
      str1,str2);
  test_zwei_string_call(&srv,str1,str2,&env);
  
  LOG("*** TEST 3 ***");
  LOG("Client(main): call test_gib_string with a = 42, instr = '%s'",str1);
  test_gib_string_call(&srv,42,str1,&outstr,&env);
  if (outstr)
    LOG("Client(main): returned, outstr = '%s'",outstr);
  else
    LOG("Client(main): returned, no outstr!");
  
  LOG("*** TEST 4 ***");
  LOG("Client(main): call test_drei_string(max-len 256)");
  LOG("  with instr = '%s'", str3);
  test_drei_string_call(&srv, str3, &env);
  
  LOG("*** TEST 5 ***");
  LOG("Client(main): call test_vier_string(max-len 256, ref)");
  LOG("  with instr = '%s'", str4);
  test_vier_string_call(&srv, str4, &env);
  
  LOG("*** TEST 6 ***");
  LOG("Client(main): call test_fuenf_string(max-len 100, ref)");
  LOG("  with instr = '%s'", str5);
  test_fuenf_string_call(&srv, str5, &env);
  
  LOG("*** TEST 7 ***");
  LOG("Client(main): call structs_getdents with fd=43");
  count = sizeof(struct test_dirent);
  ret = test_getdents_call(&srv, 43, &dirp, &count, &env);
  LOG("Client(main): returned: { %d, %d, %d, %s }, %d, ret=%d",
      (int)dirp.d_ino, (int)dirp.d_off, dirp.d_reclen, dirp.d_name, count, ret);
  
  l4_sleep(2000);
  l4util_reboot();
  
  return 0;
}

