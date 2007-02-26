#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include "test-server.h"
#include <stdlib.h> // needed for malloc
#include <stdio.h>

l4_ssize_t l4libc_heapsize = 1024*1024;

char LOG_tag[9] = "dcetst";

void *CORBA_alloc(unsigned long size) 
{
  return malloc(size);
}

void CORBA_free(void* mem) 
{
  free(mem);
}

CORBA_void test_ein_string_component(CORBA_Object _dice_corba_obj,
    const_CORBA_char_ptr str1,
    CORBA_Server_Environment *_dice_corba_env) 
{
  LOG("Server(test_ein_string): str1 = '%s'",str1);
}


CORBA_void test_zwei_string_component(CORBA_Object _dice_corba_obj,
    const_CORBA_char_ptr str1,
    const_CORBA_char_ptr str2,
    CORBA_Server_Environment *_dice_corba_env) 
{
  LOG("Server(test_zwei_string): str1 = '%s', str2 = '%s'",str1,str2);
}


CORBA_void test_gib_string_component(CORBA_Object _dice_corba_obj,
    CORBA_long a,
    const_CORBA_char_ptr instr,
    CORBA_char_ptr *outstr,
    CORBA_Server_Environment *_dice_corba_env) 
{
  LOG("Server(test_gib_string): a = %lu, instr = '%s'",a,instr);
  *outstr = "Nimm dies!";
  LOG("Server(test_gib_string): set outstr to '%s'",*outstr);
}

CORBA_void test_drei_string_component(CORBA_Object _dice_corba_obj,
    const_CORBA_char_ptr str,
    CORBA_Server_Environment *_dice_corba_env)
{
  LOG("Server(test_drei_sring): str= '%s'", str);
}

CORBA_void test_vier_string_component(CORBA_Object _dice_corba_obj,
    const_CORBA_char_ptr refstr,
    CORBA_Server_Environment *_dice_corba_env)
{
  LOG("Server(test_vier_string): refstr= '%s'", refstr);
}

CORBA_void test_fuenf_string_component(CORBA_Object _dice_corba_obj,
    const_CORBA_char_ptr refstr2,
    CORBA_Server_Environment *_dice_corba_env)
{
  LOG("Server(test_fuenf_string): refstr2= '%s'", refstr2);
}

CORBA_int
test_getdents_component(CORBA_Object _dice_corba_obj,
    CORBA_long fd,
    struct test_dirent *dirp,
    CORBA_unsigned_int *count,
    CORBA_Server_Environment *_dice_corba_env)
{
  dirp->d_ino = 12;
  dirp->d_off = 34;
  dirp->d_reclen = 56;
  strcpy(dirp->d_name, "hurra!");

  *count = sizeof(long)+sizeof(long)+sizeof(unsigned short)+7;
  return *count;
}

int main(int argc, char **argv) 
{
  CORBA_Server_Environment env = dice_default_server_environment;
  env.malloc = (dice_malloc_func)malloc;
  env.free = (dice_free_func)free;
  
  LOG("starting server");
  if (!names_register((const char*)LOG_tag)) {
      LOG("Server(main): can't register at names");
      return -1;
  }
  LOG("Server(main): registered name... entering server loop");
  test_server_loop(&env);
  return 0;
}


