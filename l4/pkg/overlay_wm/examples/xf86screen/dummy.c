/*
 * \brief   Dummy implementation of some required symbols
 * \date    2003-08-21
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * Some used libs require symbols which are not provided by the
 * X-xserver. So we have to provide it here.
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the Overlay WM package, which is distributed
 * under the  terms  of the GNU General Public Licence 2. Please see
 * the COPYING file for details.
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

void LOG_logL(void);
void LOG_logL(void) {}
