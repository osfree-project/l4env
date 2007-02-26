PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2 amd64-l4v2
TARGET = libinput.a
SRC_C = emul_time.c emul_time_jiffies.c emul_irq.c emul_wait.c l4evdev.c init.c proxy.c

PRIVATE_INCDIR = $(PKGDIR)/lib/include

DEFINES = -D__KERNEL__ -DL4INPUT
WARNINGS = -Wall -Wstrict-prototypes

# XXX circumvent gcc-2.95 's dullness
CPPFLAGS = -include $(PKGDIR)/lib/include/prevent_warnings.h

# gather Linux sources
VPATH += $(PKGDIR)/lib/contrib

SRC_C += input/input.c input/keyboard/atkbd.c input/mouse/psmouse-base.c \
         input/mouse/logips2pp.c input/mouse/synaptics.c input/mouse/alps.c \
         input/serio/i8042.c input/serio/serio.c input/serio/libps2.c \
	 input/misc/pcspkr.c

include $(L4DIR)/mk/lib.mk

ifdef DEBUG
  DEFINES += -DDEBUG_ERRORS -DDEBUG_ASSERTIONS
endif
