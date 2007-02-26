/*
 * \brief   Interface of DOpE Label widget module
 * \date    2003-05-15
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

struct label_methods;
struct label_data;

#define LABEL struct label

#if !defined(VARIABLE)
struct variable;
#define VARIABLE struct variable
#endif

struct label {
	struct widget_methods *gen;
	struct label_methods  *lab;
	struct widget_data    *wd;
	struct label_data     *ld;
};

struct label_methods {
	void      (*set_text)  (LABEL *, char *new_txt);
	char     *(*get_text)  (LABEL *);
	void      (*set_font)  (LABEL *, s32 new_font_id);
	s32       (*get_font)  (LABEL *);
	void      (*set_align) (LABEL *, char *align);
	char     *(*get_align) (LABEL *);
	void      (*set_var)   (LABEL *, VARIABLE *v);
	VARIABLE *(*get_var)   (LABEL *);
};

struct label_services {
	LABEL *(*create) (void);
};
