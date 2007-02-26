/* $Id$ */
/**
 * \file	loader/include/l4/loader/loader.h
 * 
 * \date	06/15/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * \brief	Loader library interface */

#ifndef _LOADER_LOADER_H
#define _LOADER_LOADER_H


/** The loader library sends an L4_LOADER_COMPLETE request to the loader
 * to initiate an completion */
#define L4_LOADER_COMPLETE	0x12348765

/** The loader library answers with L4_LOADER_ERROR to the loader if there
 * where some errors inside the loader library */
#define L4_LOADER_ERROR		0x43215678

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
 * This function is called by the Loader server. It's task is to attach all
 * regions of the infopage to our address space so that the region mapper can
 * page the sections later (after the region mapper pager thread is started.
 *
 * \param infopage	L4 environment infopage */
void
l4loader_init(void *infopage);

#endif

