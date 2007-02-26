/* $Id$ */
/*****************************************************************************/
/**
 * \file	dde_linux/examples/sound/my_cfg.h
 *
 * \brief	Sound Server Configuration
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 * Insert your configuration parameters before compilation.
 */
/*****************************************************************************/

/** Tag for debug logging at logserver
 * PUT your desired server log tag here. Only 8 characters allowed! */
char LOG_tag[9] = "snd-foo ";

/** \name Configuration
 * PUT your desired values here
 * @{ */
#define VMEM_SIZE	(1024*64)	/**< vmalloc() memory */
#define KMEM_SIZE	(1024*256)	/**< kmalloc()/get_page() memory */
/* @} */
