PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

# libcrt0.o.a is needed by the kaffe-l4 port (libtool story)
TARGET		= libcrt0.o.a
MODE_x86-l4v2	= sigma0
MODE		= libc
LIBCBACKEND_LIB =
SRC_C		= construction.c support.c
SRC_S		= crt0_x.S
SYSTEMS		= x86 arm

vpath %.ld $(PKGDIR)/lib/src/ARCH-$(ARCH)
vpath %.S  $(PKGDIR)/lib/src/ARCH-$(ARCH)

INSTALL_TARGET	= $(MYOBJS) main_stat.ld main_rel.ld main_dyn.ld libcrt0.o.a
MYOBJS		= crt0.o
KEEP_ON_CLEAN	= $(MYOBJS)

include $(L4DIR)/mk/lib.mk

crt0.o: crt0_x.o construction.s.o support.s.o
	@$(LINK_MESSAGE)
	$(VERBOSE)$(LD) -r $(filter %.o,$^) -o $@

