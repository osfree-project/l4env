PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
SYSTEMS         := x86-l4v2 x86-l4x0
TARGET		= libl4sys.a

SRC_C		= ipc.c syscalls.c syscalls_indirect.c

# Add the extra object file, which defines the 
# absolute system-call addresses to libl4sys.a
OBJS_libl4sys.a += syscalls_direct.o

include $(L4DIR)/mk/lib.mk

# FUN
# create an object file from the linker 
# script direct_calls.o.in
syscalls_direct.o: ../syscalls_direct.ld
	@$(GEN_MESSAGE)
	$(VERBOSE)$(LD) -r $< -o $@

