#ifndef _FILE_TABLE_H_
#define _FILE_TABLE_H_

#include <l4/dietlibc/io-types.h>

extern object_id_t cwd;  // current working directory

int ft_get_next_free_entry(void);
void ft_free_entry(int i);
void ft_fill_entry(int i, file_desc_t fd_s);
int ft_is_open(int i);
file_desc_t ft_get_entry(int i);
void ft_init(void);

#endif
