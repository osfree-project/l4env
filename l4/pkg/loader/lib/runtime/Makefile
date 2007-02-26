PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libloader.s.so
MODE		= l4env
LDFLAGS		= -Bsymbolic
SYSTEMS		= x86-l4v2 x86-l4x0

INSTALLFILE_LIB_LOCAL = $(STRIP) --strip-unneeded $(1) -o $(2) && chmod 755 $(2)

OBJS 		= main.s.o helper_dyn.s.o
LIBS		= --whole-archive \
		    -lsemaphore.p \
		    -lthread.p \
		    -ldm_generic.p \
		    -ldm_mem.p \
		    -lslab.p \
		    -ll4rm.p \
		    -ll4env_loaderruntime.p \
		    -llogserver.p \
		    -lnames.p \
		    -lrmgr.p \
		    -lgeneric_ts.p \
		    -lloaderif.p \
		  --no-whole-archive \
		    -ll4util.p

include $(L4DIR)/mk/lib.mk
