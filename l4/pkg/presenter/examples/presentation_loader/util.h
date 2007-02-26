#include <string.h>
#include <dirent.h>

#define ABSPATH(x)    ((x)[0] == '/')

#define DIRSEP  '/'
#define ISDIRSEP(c)     ((c) == '/')
#define PATHSEP(c)      (ISDIRSEP(c) || (c) == 0)

#define PRES_CONFIG_EXTENSION ".pres"

int absolute_pathname (char *);
char *base_pathname (char *);
int select_pres (const struct dirent *);
int config_choice_dialog(const char *, struct dirent **, int n);




