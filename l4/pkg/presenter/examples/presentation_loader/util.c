#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

/* Return 1 if STRING contains an absolute pathname, else 0.  */
int absolute_pathname (char *string) {

	if (string == 0 || *string == '\0')
		return (0);

	if (ABSPATH(string))
	        return (1);

	if (string[0] == '.' && PATHSEP(string[1]))   /* . and ./ */
	        return (1);

	if (string[0] == '.' && string[1] == '.' && PATHSEP(string[2]))       /* .. and ../ */
	        return (1);

	return (0);
}


/* Return the `basename' of the pathname in STRING (the stuff after the
 *    last '/').  If STRING is not a full pathname, simply return it. */
char *base_pathname (char *string) {
	char *p;

	if (absolute_pathname (string) == 0)
	        return (string);

        p = (char *)strrchr (string, '/');
        return (p ? ++p : string);
}

int select_pres (const struct dirent *curr_dir) {

	if (!strstr(curr_dir->d_name,PRES_CONFIG_EXTENSION))
		return 0;
	else 
		return 1;
}

int config_choice_dialog(const char *presentation_path, struct dirent **namelist, int n) {
	int i,choice;
	char curr_line[1024];

	fprintf(stdout,"Current Path: %s\nPlease choose one of the following presentations:\n",presentation_path);

        for (i=0;i<n;i++) {
 	       fprintf(stdout,"(%d) %s\n",i+1,namelist[i]->d_name);
        }

        fprintf(stdout,"Your choice - a number between 1 and %d or q to quit: ",n);

        while (1) {

        	fgets(curr_line,sizeof(curr_line),stdin);

		fflush(stdin);

		if (curr_line[0] == 113) {
			choice=0;
			break;
		}

		choice = atoi(curr_line);

		if (choice > 0 && choice <= n) {
			break;
		}
		else {
			fprintf(stdout,"Your choice - a number between 1 and %d or q to quit: ",n);
		}


	} 

	return choice;
}
