/*
  ogmmerge -- utility for splicing together ogg bitstreams
      from component media subtypes

  ogmmerge.h
  general class and type definitions

  Written by Moritz Bunkus <moritz@bunkus.org>
  Based on Xiph.org's 'oggmerge' found in their CVS repository
  See http://www.xiph.org

  Distributed under the GPL
  see the file COPYING for details
  or visit http://www.gnu.org/copyleft/gpl.html
*/
#ifndef __COMMON_H
#define __COMMON_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <ogg/ogg.h>

#define VERSIONINFO "ogmtools v" VERSION

  extern int verbose;

/* errors */
#define EMOREDATA    -1
#define EMALLOC      -2
#define EBADHEADER   -3
#define EBADEVENT    -4
#define EOTHER       -5

/* types */
#define TYPEUNKNOWN   0
#define TYPEOGM       1
#define TYPEAVI       2
#define TYPEWAV       3
#define TYPESRT       4
#define TYPEMP3       5
#define TYPEAC3       6
#define TYPECHAPTERS  7
#define TYPEMICRODVD  8
#define TYPEVOBSUB    9

#define u_int16_t unsigned short
#define u_int32_t unsigned long
#define u_int64_t unsigned long long

  u_int16_t get_uint16 (const void *buf);
  u_int32_t get_uint32 (const void *buf);
  u_int64_t get_uint64 (const void *buf);
  void put_uint16 (void *buf, u_int16_t);
  void put_uint32 (void *buf, u_int32_t);
  void put_uint64 (void *buf, u_int64_t);

#ifdef __cplusplus
}
#endif

#endif
