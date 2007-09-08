/*
 * \brief   DDE configuration for using STPM with dde_linux.
 * \date    2004-06-02
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2004  Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


#define VMEM_SIZE   (1024*64)   /**< vmalloc() memory */
#define KMEM_SIZE   (1024*256)  /**< kmalloc()/get_page() memory */
