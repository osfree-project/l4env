PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

MODE_x86-l4v2	= sigma0
MODE		= libc
LIBCBACKEND_LIB = 
SRC_C		= construction.c support.c
SYSTEMS		= x86 arm

vpath %.ld $(PKGDIR)/lib/src/ARCH-$(ARCH)
vpath %.S $(PKGDIR)/lib/src/ARCH-$(ARCH)

INSTALL_TARGET	= $(OBJS) main_stat.ld main_rel.ld
OBJS		= crt0.o
KEEP_ON_CLEAN	= $(OBJS)

include $(L4DIR)/mk/lib.mk

crt0.o: crt0_x.o construction.o support.o
	@$(LINK_MESSAGE)
	$(VERBOSE)$(LD) -r $^ -o $@

