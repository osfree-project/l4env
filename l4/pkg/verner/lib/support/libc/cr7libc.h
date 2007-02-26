#ifndef DIETLIBC_CR7_H
#define DIETLIBC_CR7_H 1

/* prototyps */
int cr7_printf(const char *format,...);
int cr7_sprintf(char *dest,const char *format,...);
int cr7_snprintf(char *s, int size, const char *fmt, ...);
inline int cr7_toupper(int ch);
double cr7_strtod(const char *nptr, char **endptr);
long int cr7_strtol(const char *nptr, char **endptr, int base);
unsigned long int cr7_strtoul(const char *nptr, char **endptr, int base);

#endif
