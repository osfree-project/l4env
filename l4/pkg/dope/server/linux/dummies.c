/*
 * \brief	DOpE dummy init functions for non-existent modules
 * \date	2002-11-13
 * \author	Norman Feske <nf2@inf.tu-dresden.de>
 */

#include "dope-config.h"

int init_pslim_server(struct dope_services *d);

int init_pslim_server(struct dope_services *d) {
	return 1;
}

