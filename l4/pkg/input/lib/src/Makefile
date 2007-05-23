PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2 amd64-l4v2 arm-l4x0
TARGET = libinput.a
SRC_C = emul_time.c emul_time_jiffies.c emul_irq.c emul_wait.c emul_res.c \
        l4evdev.c init.c proxy.c

PRIVATE_INCDIR = $(PKGDIR)/lib/include

DEFINES = -D__KERNEL__ -DL4INPUT
WARNINGS = -Wall -Wstrict-prototypes

# gather Linux sources
VPATH += $(PKGDIR)/lib/contrib

SRCALL = input/input.c input/keyboard/atkbd.c input/mouse/psmouse-base.c \
         input/mouse/logips2pp.c input/mouse/synaptics.c input/mouse/alps.c \
         input/serio/serio.c input/serio/libps2.c

SRC_C_x86-l4v2   += $(SRCALL) input/serio/i8042.c input/misc/pcspkr.c ux.c
SRC_C_amd64-l4v2 += $(SRCALL) input/serio/i8042.c input/misc/pcspkr.c
SRC_C_arm-l4x0   += $(SRCALL) input/serio/ambakmi.c

include $(L4DIR)/mk/lib.mk
