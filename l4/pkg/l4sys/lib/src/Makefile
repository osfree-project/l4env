PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
SYSTEMS         := x86-l4v2 x86-l4x0
TARGET		= libl4sys.a

SRC_C		= ipc.c syscalls.c x2_like.c 

# Add the extra object file, which defines the 
# absolute system-call addresses to libl4sys.a
OBJS_libl4sys.a += direct_calls.o

include $(L4DIR)/mk/lib.mk

# FUN
# create an object file from the linker 
# script direct_calls.o.in
direct_calls.o: ../direct_calls.o.in
	$(LD) -r $< -o $@

