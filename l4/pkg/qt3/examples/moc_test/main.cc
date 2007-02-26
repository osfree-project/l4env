/* $Id$ */
/*****************************************************************************/
/**
 * \file   examples/thread_test/main.cc
 * \brief  MOC test case.
 *
 * \date   10/12/2004
 * \author Carsten Weinhold <weinhold@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004-2006 Technische Universitaet Dresden
 * This file is part of the Qt3 port for L4/DROPS, which is distributed under
 * the terms of the GNU General Public License 2. Please see the COPYING file
 * for details.
 */

#include "class_c.h"

int main(int argc, char **argv) {

  C *c = new C;

  QObject::connect(c, SIGNAL(signal(int)), c, SLOT(slot(int)) );

  c->work(42);

  return 0;  
}

#define PROTO_IMPL(x) x; x

PROTO_IMPL(void drops_get_mouse(int*, int*, int*)){};
PROTO_IMPL(void drops_get_keyboard(int*, int*)){};
PROTO_IMPL(void drops_get_keymodifiers(int*, int*, int*, int*, int*)){};
PROTO_IMPL(void drops_qt_key(int)){};
PROTO_IMPL(void drops_init_mouse()){};

extern "C" {
  PROTO_IMPL(void qws_gfx_con_init(void)){};
}

