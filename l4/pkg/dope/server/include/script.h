/*
 * \brief   Interface of the DOpE command interpreter
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



struct script_services {
	void *(*reg_widget_type)   (char *widtype_name, void *(*create_func)(void));
	void  (*reg_widget_method) (void *widtype, char *desc, void *methadr);
	void  (*reg_widget_attrib) (void *widtype, char *desc, void *get, void *set, void *update);
	char *(*exec_command)      (u32 app_id, const char *cmd);
};


