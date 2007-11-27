#include <l4/presenter/presenter-client.h>
#include <l4/presenter/presenter_lib.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include <l4/sys/types.h>
#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/sys/syscalls.h>
#include <l4/names/libnames.h>

#include "util.h"
#include "rel2abs.h"

#define PRESENTATION_PATH_MAX_LEN 1024

l4_threadid_t id = L4_INVALID_ID;
static CORBA_Object _dice_corba_obj = &id;
// CORBA_Environment _dice_corba_env = dice_default_environment;

int main(int argc, char *argv[]) {
	int key,n,len,choice;
	DIR *dir;
	struct dirent **namelist;
	char presentation_path[PRESENTATION_PATH_MAX_LEN], cwd_buf[PRESENTATION_PATH_MAX_LEN]; 
        l4_threadid_t me;
        struct stat buf;
		CORBA_Environment _dice_corba_env = dice_default_environment;

        if (stat("/proc/l4", &buf)) {
                fprintf(stderr, "This binary requires L4Linux!\n");
                exit(-1);
        }

	me = l4_myself();

	if (argc > 2) {
		fprintf(stderr, "Presentation Loader\n"
	        "Usage:\n"
	        "[path to presentations]\n");
	        return -1;
	}

	if (argc == 1) {
                if(!getcwd(presentation_path,PRESENTATION_PATH_MAX_LEN)) {
		        fprintf(stderr,"error on reading current working directory, errno: %d\n",errno);
                	return -1;
		}
		len = strlen (presentation_path);
		strcpy(presentation_path+len,"/");
        }
        else {
                /* true if not an absolute path */
                if (argv[1][0] != '/') {
                        if (!getcwd(cwd_buf,PRESENTATION_PATH_MAX_LEN)) {
                                fprintf(stderr,"error on reading current working directory, errno: %d\n",errno);
                                return -1;
                        }

                        if (!rel2abs(argv[1],cwd_buf,presentation_path,PRESENTATION_PATH_MAX_LEN)) {
                                fprintf(stderr,"error on creating absolute path\n");
                                return -1;
                        }
                }
                else
                        strcpy(presentation_path,argv[1]);

        }


	/* if path does not contain name of presentation config file */
	if (!strstr(presentation_path,PRES_CONFIG_EXTENSION)) {

		dir = opendir(presentation_path);

		if (!dir) {
			fprintf(stderr,"error on reading %s\n",presentation_path);
			return -1;
		}

		n = scandir(presentation_path,&namelist,select_pres,alphasort);

		/* did not find a pres config file in path */
		if (n == 0) {
			fprintf(stdout,"Sorry, did not find presentation config files in %s\n",presentation_path);
			return -1;
		}
		
		/* found only one pres config file in path */
		if (n == 1) {
			strcat(presentation_path,namelist[0]->d_name);
		}
		else {
			choice = config_choice_dialog(presentation_path,namelist,n);

			if (!choice) return 0;

			strcat(presentation_path,namelist[choice-1]->d_name);
		}

	}

	while (names_waitfor_name(PRESENTER_NAME,_dice_corba_obj,40000)==0) {
		fprintf(stdout,"waiting for presenter\n");
	}

	fprintf(stdout,"found presenter, loading %s\n",presentation_path);

	key = presenter_load_call(_dice_corba_obj,presentation_path,&_dice_corba_env);

	if (key > 0) {
		fprintf(stdout,"presentation sucessfully loaded, try to show it\n");
		presenter_show_call(_dice_corba_obj,key,&_dice_corba_env);
	}
	else fprintf(stderr,"error, could not load presentation\n");
						        
	return 0;
}
