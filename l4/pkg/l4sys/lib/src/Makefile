PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
SYSTEMS         := x86-l4v2 x86-l4x0 x86-l4x2 arm-l4x0
TARGET		= libl4sys.a

ifeq ($(L4API),l4x2)
SRC_S		= L4API-l4x2/ia32-syscall-stubs.S
SRC_CC		= L4API-l4x2/ia32-syscall-init.cc

else
ifeq ($(ARCH),arm)
SRC_C		=
SRC_S		:= ARCH-arm/atomic_ops_s.S

vpath %.ld ../ARCH-arm

else
SRC_C		= ipc.c syscalls.c syscalls_indirect.c

# Add the extra object file, which defines the 
# absolute system-call addresses to libl4sys.a
OBJS_libl4sys.a += syscalls_direct.o
endif
endif

include $(L4DIR)/mk/lib.mk

vpath %.ld ..

# FUN
# create an object file from the linker 
# script direct_calls.o.in
atomic_ops.o syscalls_direct.o: %.o: %.ld
	@$(GEN_MESSAGE)
	$(VERBOSE)$(LD) -r $< -o $@
	
