/*
 * \brief   Dummy implementation of some required symbols
 * \date    2003-08-21
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * Some used libs require symbols which are not provided by the
 * X-xserver. So we have to provide it here.
 */

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

void l4env_err_ipcstrings(void);
void l4env_err_ipcstrings(void) {}

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

