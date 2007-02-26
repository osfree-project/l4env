/*!
 * \file   io_rtns.h
 * \brief  OshKosh/rt-fs specific plugin for file-I/O
 * \date   08/13/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of the VERNER, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */
#ifndef __IO_RTNS_H_
#define __IO_RTNS_H_

#include <sys/stat.h>

/*
 * working functions 
 */
int io_rtns_open (const char *__name, int __mode, ...);
int io_rtns_close (int __fd);

unsigned long io_rtns_read (int __fd, void *__buf, unsigned long __n);

/* Unused */
unsigned long io_rtns_write (int __fd, void *__buf, unsigned long __n);

long io_rtns_lseek (int __fd, long __offset, int __whence);

/* File size: MP3 */
int io_rtns_fstat (int __fd, struct stat *buf);

/*
 * not implemented
 */
int io_rtns_ftruncate (int __fd, unsigned long __offset);

#endif
