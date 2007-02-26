/* $Id$ */
/**
 * \file	loader/include/loader.h
 * \brief	Loader library interface
 * 
 * \date	06/15/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef _LOADER_LOADER_H
#define _LOADER_LOADER_H

#include <l4/sys/compiler.h>

/** The loader library sends an L4_LOADER_COMPLETE request to the loader
 * to initiate an completion */
#define L4LOADER_COMPLETE	0x12348765

/** The loader library answers with L4_LOADER_ERROR to the loader if there
 * where some errors inside the loader library */
#define L4LOADER_ERROR		0x43215678

#define L4LOADER_STOP		0x00000001

EXTERN_C_BEGIN

/** Init L4environment library
 *
 * This is the second entry point from the Loader to the loader library.
 * After all program sections are registered at the L4 region manager and
 * all sections are relocated, initialize the L4 environment. Finally call
 * multiboot_main() or main() depending on which function is available. */
void
l4env_init(void);

/** Init loader libary
 *
 * This function is called by the Loader server. Its task is to attach all
 * regions of the infopage to our address space so that the region mapper can
 * page the sections later (after the region mapper pager thread is started.
 *
 * \param infopage	L4 environment infopage */
void
l4loader_init(void *infopage);

void
l4loader_attach_relocateable(void *infopage);

EXTERN_C_END

#endif

