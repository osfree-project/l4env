PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= cpu_reserved
# the default relocation address. This may be superseded by a STATIC file.
DEFAULT_RELOC	= 0x009b0000

# list your .c files here
SRC_C		= main.c sched.c corba.c granularity.c watch.c monitor.c
SERVERIDL	= cpu_reserve.idl
LIBS		= -lparsecmdline -lcpu_reserve -lrt_mon_client

include $(L4DIR)/mk/prog.mk
