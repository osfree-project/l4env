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

