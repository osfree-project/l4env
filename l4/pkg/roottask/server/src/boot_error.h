#ifndef BOOT_ERROR_H
#define BOOT_ERROR_H

void boot_wait(void);
void boot_warning(const char *fmt, ...);
void boot_error(const char *fmt, ...);
void boot_panic(const char *fmt, ...);

#endif
