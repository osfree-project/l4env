#ifndef __DIETLIBC_LIB_SERVER_NAME_SERVER_SRC_PATHNAMES_H_
#define __DIETLIBC_LIB_SERVER_NAME_SERVER_SRC_PATHNAMES_H_

char * get_first_path(const char * pathname);
char * get_remainder_path(const char * pathname);
int is_absolute_path(const char * pathname);
int is_up_path(const char * pathname);

#endif
