#include "presenter_exec-server.h"
#include <l4/presenter/presenter_exec_lib.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include <l4/names/libnames.h>

#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>

l4_ssize_t l4libc_heapsize = 6*1024*1024;

static void signal_handler(int code){

        names_unregister(PRESENTER_EXEC);
        fprintf(stderr,"presenter_exec aborted (signal %d)\n",code);
        exit(-1);
}


CORBA_int
presenter_exec_execvp_component(CORBA_Object _dice_corba_obj,
                                const_CORBA_char_ptr command_path,
                                const_CORBA_char_ptr argv,
                                CORBA_Server_Environment *_dice_corba_env)
{
	char *curr_argv, *space_ptr, *curr_command;
	char *parsed_argv[32];
	int j,size, result,fd;
	pid_t exec_id;
	j=0;
	size=0;
	curr_argv = (char *) strdup((char *)argv);

	fprintf(stdout,"presenter_exec called, argv: %s\n",curr_argv);

	/* open file for redirection of stderr output */
	fd = open(ERROR_LOG,O_CREAT | O_RDWR,S_IRWXU | S_IRGRP | S_IROTH);

	if (fd < 0) {
		fprintf(stderr,"presenter_exec: could not redirect stderr\n");	
	}

	/* split argv into single commands */
	while (curr_argv[0] != '\0') {
	
	/* search next space character in string */
	space_ptr = (char *) strchr(curr_argv,32);
        if (space_ptr == NULL) {
                        space_ptr = (char *) strchr(curr_argv,0);
                        if (space_ptr == NULL) break;
                }

                size = space_ptr-curr_argv+1;

                curr_command = (char *)malloc(size);

                curr_argv[size-1] = '\0';

                curr_command = strcpy(curr_command,curr_argv);

                if (curr_command == NULL) break;

                parsed_argv[j] = curr_command;

                curr_argv+=size;
                j++;
        }

	parsed_argv[j] = NULL;

	exec_id = fork();

	if (exec_id == -1) {
		 fprintf(stderr,"presenter_exec: fork exception error\n");
                 return -1;
	}

	if (exec_id == 0) {
		/* close stderr */
                close(2);

                /* redirect stderr */
                dup(fd);

                result = execvp(command_path,parsed_argv);

                fprintf(stderr,"error on execvp with result: %d, errno: %d",result,errno);
	}

	if (exec_id != -1) {
		waitpid(exec_id, NULL,0);
	}		
	
	/* close error log file descriptor */
	result = close(fd);

	if (result < 0) {
	         fprintf(stderr,"error on closing error log, error nr: %d\n",errno);
	         return -1;
	}


	return 0;
}

int main(int argc, char *argv[]) {
        l4_threadid_t me;
	struct stat buf;

	if (stat("/proc/l4", &buf)) {
                fprintf(stderr, "This binary requires L4Linux!\n");
                exit(-1);
        }

        me = l4_myself();

        printf("starting presenter_exec server\n");

        /* try to register at nameserver */
        if (names_register(PRESENTER_EXEC)==0) {
                fprintf(stderr, "Failed to register l4xexec\n");
                return -2;
        }

	signal(SIGTERM,signal_handler);
        signal(SIGINT,signal_handler);

        presenter_exec_server_loop(0);

        return 0;
}


