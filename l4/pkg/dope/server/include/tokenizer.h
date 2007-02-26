/*
 * \brief   Interface of tokenizer module
 * \date    2002-11-13
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#define TOKEN_EMPTY     0
#define TOKEN_IDENT     1       /* identifier */
#define TOKEN_STRUCT    2       /* structure element (brackets etc.) */
#define TOKEN_STRING    3       /* string */
#define TOKEN_WEIRD     4       /* weird */
#define TOKEN_NUMBER    5       /* number */
#define TOKEN_EOS       99      /* end of string */

struct tokenizer_services {
	s32 (*parse)  (const char *str,s32 max_tok,s32 *offbuf,s32 *lenbuf);
	s32 (*toktype)(const char *str,s32 offset);
};
