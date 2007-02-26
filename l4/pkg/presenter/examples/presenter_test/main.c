#include <l4/presenter/presenter-client.h>

#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>

#include <dice/dice.h>

l4_threadid_t id = L4_INVALID_ID;
static CORBA_Object _dice_corba_obj = &id;
CORBA_Environment _dice_corba_env = dice_default_environment;

int main (void) {

	char *name = "/home/js712688/test_presenter/example.pres";
	int present_key;
	
	LOG_init("pr_test");
	LOG("started presenter_test...");

	while (names_waitfor_name("PRESENT",_dice_corba_obj,40000)==0) {
                LOG("PRESENTER not found yet...");
        }
	
	LOG("try to load config file %s...",name);
	present_key=presenter_load_call(_dice_corba_obj, name,&_dice_corba_env);

	if (present_key!=-1) {
		LOG("presentation loaded with key %d",present_key);
		LOG("try to show presentation");
	    presenter_show_call(_dice_corba_obj, present_key,&_dice_corba_env);

	}
	else {
		LOG("error, could not show presentation");
	}


	return 0;

}
