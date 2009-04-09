/*
 * \date    2009-04-07
 * \author  Dirk Vogt <dvogt@os.inf.tu-dresden.de>
 */

/*
 * This file is part of the DDEUSB package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */





#ifndef L4_PKG_USB_SERVER_SRC_DEBUG_H
#define L4_PKG_USB_SERVER_SRC_DEBUG_H

#undef WARN_UNIMPL
#if DDEUSB_DEBUG
#define WARN_UNIMPL         printk("unimplemented: %s\n", __FUNCTION__)
#define DEBUG_MSG(msg, ...) printk("%s: \033[36m" msg "\033[0m\n", __FUNCTION__, ## __VA_ARGS__)
#else
#define WARN_UNIMPL
#define DEBUG_MSG(msg, ...)
#endif

#endif
