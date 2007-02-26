/*
 * \brief   Interface of DOpE Scale widget module
 * \date    2003-06-11
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

struct scale_methods;
struct scale_data;

#define SCALE struct scale

#if !defined(VARIABLE)
struct variable;
#define VARIABLE struct variable
#endif

#define SCALE_VER  0x1

struct scale {
	struct widget_methods *gen;
	struct scale_methods  *scale;
	struct widget_data    *wd;
	struct scale_data     *sd;
};

struct scale_methods {
	void      (*set_type)  (SCALE *, u32 type);
	u32       (*get_type)  (SCALE *);
	void      (*set_value) (SCALE *, float new_value);
	float     (*get_value) (SCALE *);
	void      (*set_from)  (SCALE *, float new_from);
	float     (*get_from)  (SCALE *);
	void      (*set_to)    (SCALE *, float new_to);
	float     (*get_to)    (SCALE *);
	void      (*set_var)   (SCALE *, VARIABLE *v);
	VARIABLE *(*get_var)   (SCALE *);
};

struct scale_services {
	SCALE *(*create) (void);
};
