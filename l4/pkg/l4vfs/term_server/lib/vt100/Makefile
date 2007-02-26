PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../../..

SYSTEMS = x86-l4v2 x86-l4x0

TARGET  = libvt100.a
MODE    = l4env_minimal

SRC_C   = color.c vt100.c buffer.c init.c cursor.c echo.c termcap.c  \
          rw.c getchar.c tabs.c insert.c

include $(L4DIR)/mk/lib.mk
