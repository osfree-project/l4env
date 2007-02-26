#ifndef __L4_EXEC_SERVER_EXC_OBJ_STAB_H
#define __L4_EXEC_SERVER_EXC_OBJ_STAB_H

typedef struct 
{
  unsigned long n_strx;         /* index into string table of name */
  unsigned char n_type;         /* type of symbol */
  unsigned char n_other;        /* misc info (usually empty) */
  unsigned short n_desc;        /* description field */
  unsigned long n_value;        /* value of symbol */
} __attribute__ ((packed)) stab_entry_t;

typedef struct
{
  unsigned long addr;
  unsigned short line;
} __attribute__ ((packed)) stab_line_t;

class exc_obj_stab_t
{
  public:
    exc_obj_stab_t(const char *dbg_name);
    exc_obj_stab_t(const exc_obj_stab_t&);
    ~exc_obj_stab_t();

    int add_section(const stab_entry_t *se, const char *se_str, unsigned n);
    int get_size(l4_size_t *str_size, l4_size_t *lin_size);
    int get_lines(char **str, stab_line_t **lin);
    int truncate(void);
    int print(void);
    
  private:
    inline int search_str(const char *str, unsigned len, unsigned *idx);
    inline int add_str(const char *str, unsigned len, unsigned *idx);
    int add_entry(const stab_entry_t *se, const char *se_str);
    void *realloc(l4dm_dataspace_t *ds, void *addr, l4_size_t size);
    
    unsigned	func_offset;
    unsigned	have_func;
    
    char	*str_field;
    unsigned	str_end, str_max;
    l4dm_dataspace_t str_ds;
    stab_line_t	*lin_field;
    unsigned	lin_end, lin_max;
    l4dm_dataspace_t lin_ds;

    const char *name;
};

#endif

