/*!
 * \file   zlib/lib/build/func_rename.h
 * \brief  Rename f*-funcions
 *
 * \date   05/17/2004
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 */
/* (c) 2004 Technische Universität Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __FUNC_RENAME_H_
#define __FUNC_RENAME_H_

#define FILE L4ZLIB_FILE

#define fread(buf, size, nmemb, file) \
	l4zlib_fread(buf, size, nmemb, file)

#define fwrite(buf, size, nmemb, file) \
	l4zlib_fwrite(buf, size, nmemb, file)

#define fopen(name, mode) \
	l4zlib_fopen(name, mode)

#define fdopen(fd, mode) \
	l4zlib_fdopen(fd, mode)

#define fclose(file) \
	l4zlib_fclose(file)

#define fseek(file, offset, whence) \
	l4zlib_fseek(file, offset, whence)

#define rewind(file) \
	l4zlib_rewind(file)

#define ftell(file) \
	l4zlib_ftell(file)

#define fputc(c, file) \
	l4zlib_fputc(c, file)

#define fflush(file) \
	l4zlib_fflush(file)

#define fprintf(x...) \
	l4zlib_fprintf(x)

#endif /* ! __FUNC_RENAME_H_ */
