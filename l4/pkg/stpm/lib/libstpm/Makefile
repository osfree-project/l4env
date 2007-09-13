PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= libstpm-l4.o.a libstpm-l4-tis.o.a libstpm-ux.a libstpm-emu.a

include $(PKGDIR)/../dde/linux26/Makeconf

CFLAGS  	+= -ffunction-sections
CFLAGS_tis.c	+= -std=gnu99

# list your .c files here
SRC_C_libstpm-l4.o.a		= inftpm_base.c inftpm_init.c inftpm_io.c inftpm_tl.c tpm.c ddeglue.c
SRC_C_libstpm-l4-tis.o.a	= tisglue.c tisutil.c tisexit.c tis.c
SRC_C_libstpm-ux.a		= uxglue.c
SRC_C_libstpm-emu.a		= emuglue.c

PRIVATE_INCDIR += $(PKGDIR)/include 		\
                 $(PKGDIR)/contrib/inftpm 	\
                 $(PKGDIR)/contrib/tpm 		\
                 $(PKGDIR)/contrib/oslo/include \
				 $(PKGDIR)/contrib/tis_oslo   

VPATH         += $(PKGDIR)/contrib/inftpm
VPATH         += $(PKGDIR)/contrib/tpm
VPATH         += $(PKGDIR)/contrib/oslo
VPATH         += $(PKGDIR)/contrib/tis_oslo
		 
WARNINGS += -Wall -Wstrict-prototypes

include $(L4DIR)/mk/lib.mk
