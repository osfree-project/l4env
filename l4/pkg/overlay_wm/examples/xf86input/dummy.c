/*
 * \brief   Dummy implementation of some required symbols
 * \date    2003-08-21
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * Some used libs require symbols which are not provided by the
 * X-xserver. So we have to provide it here.
 */

void LOG_flush(void);
void LOG_flush(void) {}

void printf(void);
void printf(void) {}

extern void* xf86malloc(int size);

void *malloc(int size);
void *malloc(int size) {
	return xf86malloc(size);
}

char *strcpy(char *to, const char *from);
char *strcpy(char *to, const char *from) {
	register char *ret = to;
	while ((*to++ = *from++) != 0);
	return ret;
}

int strlen(const char *string);
int strlen(char *string) {
	char *ret = string;
	while (*string++);
	return string - 1 - ret;
}

//void l4env_err_ipcstrings(void);
//void l4env_err_ipcstrings(void) {}

char *strncpy(char *to, const char *from, int count);
char *strncpy(char *to, const char *from, int count) {
	register char *ret = to;

	while (count > 0) {
		count--;
		if ((*to++ = *from++) == '\0')
			break;
	}

	while (count > 0) {
		count--;
		*to++ = '\0';
	}
	return ret;
}


/*******************************************
 * FUNCTIONS TO MAKE LIBTHREAD_LINUX HAPPY *
 *******************************************/

extern int xf86close(int);
int close(int fh);
int close(int fh) {
	return xf86close(fh);
}

extern void xf86free(void *);
void free(void *addr);
void free(void *addr) {
	xf86free(addr);
}

extern int xf86ioctl(int, unsigned long, void *);
int ioctl(int fh, unsigned long opcode, void *argp);
int ioctl(int fh, unsigned long opcode, void *argp) {
	return xf86ioctl(fh,opcode,argp);
}

extern void *xf86memset(void*,int,int);
void *memset(void *addr, int val, unsigned int num);
void *memset(void *addr, int val, unsigned int num) {
	return xf86memset(addr,val,num);
}

/*
 * NOTE: libthread_linux uses open only with two arguments!
 */
extern int xf86open(const char*, int,...);
int open(const char *pathname, int flags);
int open(const char *pathname, int flags) {
	return xf86open(pathname,flags);
}

void putchar(void);
void putchar(void) {}

void l4_oskit_support_error_init(void);
void l4_oskit_support_error_init(void) {}

void l4env_strerror(void);
void l4env_strerror(void) {}
