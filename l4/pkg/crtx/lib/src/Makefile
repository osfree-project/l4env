PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

# libcrt0.o.a is needed by the kaffe-l4 port (libtool story)
TARGET		= libcrt0.o.a
MODE_x86-l4v2	= sigma0
MODE		= libc
LIBCBACKEND_LIB =
SRC_C		= construction.c support.c
SRC_S_libcrt0.o.a = crt0_x.S
SRC_S_crt0.o    = crt0_x.S
SRC_S_crt0.s.o  = crt0_x.S
SYSTEMS		= x86 arm amd64

vpath %.ld $(PKGDIR)/lib/src/ARCH-$(ARCH)
vpath %.S  $(PKGDIR)/lib/src/ARCH-$(ARCH)

INSTALL_TARGET	= $(MYOBJS) main_stat.ld main_rel.ld main_dyn.ld libcrt0.o.a
MYOBJS		= crt0.o crt0.s.o
KEEP_ON_CLEAN	= $(MYOBJS)

include $(L4DIR)/mk/lib.mk

crt0.o: crt0_x.o construction.o support.o
	@$(LINK_MESSAGE)
	$(VERBOSE)$(LD) $(LDFLAGS) -m $(LD_EMULATION) -r $(filter %.o,$^) -o $@

crt0.s.o: crt0_x.s.o construction.s.o support.s.o .general.d
	@$(LINK_MESSAGE)
	$(VERBOSE)$(LD) $(LDFLAGS) -m $(LD_EMULATION) -r $(filter %.o,$^) -o $@
