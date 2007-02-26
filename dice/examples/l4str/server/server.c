#include <l4/names/libnames.h>
#include <l4/oskit10_l4env/support.h>
#include "test-server.h"
#include <stdio.h>


CORBA_void test_ein_string_component(CORBA_Object *_dice_corba_obj,
                                     CORBA_char_ptr str1,
                                     CORBA_Environment *_dice_corba_env) {
	printf("Server(test_ein_string): str1 = '%s'\n",str1);
}


CORBA_void test_zwei_string_component(CORBA_Object *_dice_corba_obj,
                                      CORBA_char_ptr str1,
                                      CORBA_char_ptr str2,
                                      CORBA_Environment *_dice_corba_env) {
	printf("Server(test_zwei_string): str1 = '%s', str2 = '%s'\n",str1,str2);
}


CORBA_void test_gib_string_component(CORBA_Object *_dice_corba_obj,
                                     CORBA_long a,
                                     CORBA_char_ptr instr,
                                     CORBA_char_ptr *outstr,
                                     CORBA_Environment *_dice_corba_env) {
	printf("Server(test_gib_string): a = %lu, instr = '%s'\n",a,instr);
	*outstr = "Nimm dies!";
	printf("Server(test_gib_string): set outstr to '%s'\n",*outstr);
}

CORBA_void test_drei_string_component(CORBA_Object *_dice_corba_obj,
    CORBA_char_ptr str,
    CORBA_Environment *_dice_corba_env)
{
  printf("Server(test_drei_sring): str= '%s'\n", str);
}

CORBA_void test_vier_string_component(CORBA_Object *_dice_corba_obj,
    CORBA_char_ptr refstr,
    CORBA_Environment *_dice_corba_env)
{
  printf("Server(test_vier_string): refstr= '%s'\n", refstr);
}

CORBA_void test_fuenf_string_component(CORBA_Object *_dice_corba_obj,
    CORBA_char_ptr refstr2,
    CORBA_Environment *_dice_corba_env)
{
  printf("Server(test_fuenf_string): refstr2= '%s'\n", refstr2);
}

int main(int argc, char **argv) {
	OSKit_libc_support_init(500*1024);
		
	if (!names_register("dcetst")) {
		printf("Server(main): can't register at names\n");
		return -1;
	}
	printf("Server(main): registered name... entering server loop\n");
	test_server_loop(NULL);
	return 0;
}


