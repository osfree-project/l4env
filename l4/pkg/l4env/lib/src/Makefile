PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET_x86-l4v2	= libl4env.a libl4env_loaderruntime.a
TARGET_x86-l4x0	= $(TARGET_x86-l4v2)
TARGET_x86	= libl4env_err.a
BUILD_PIC	= libl4env_loaderruntime.a libl4env_err.a
MODE 		= l4env
SYSTEMS		= x86-l4v2 x86-l4x0 x86

vpath %.S $(PKGDIR)/lib/src/ARCH-$(ARCH)

PRIVATE_INCDIR	= ../../include

# error library
SRC_C_libl4env_err.a	= errstr.c
SRC_C_libl4env_err.p.a  = errstr.c

# for L4Env application
SRC_C_libl4env.a	= startup.c environment.c errstr-l4ipc.c

# for including in loader runtime library
SRC_C_libl4env_loaderruntime.p.a = environment.c

include $(L4DIR)/mk/lib.mk
