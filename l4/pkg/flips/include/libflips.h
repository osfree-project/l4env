/** FUNCTIONS FOR ACCESSING PROC ENTRIES ***/

extern int flips_proc_read(const char *path, char *dst,
                           int offset, int len);
extern int flips_proc_write(const char *path, char *src,
                            int len);

extern unsigned int flips_inet_addr_type(unsigned int addr);

