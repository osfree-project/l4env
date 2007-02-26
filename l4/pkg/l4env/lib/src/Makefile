PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET_x86-l4v2	= libl4env.a libl4env_loaderruntime.a
TARGET_x86-l4x2	= $(TARGET_x86-l4v2)
TARGET_arm-l4x0	= $(TARGET_x86-l4v2)
TARGET_x86	= libl4env_err.a
TARGET_arm	= $(TARGET_x86)
TARGET_amd64-l4v2 = libl4env.a libl4env_loaderruntime.a libl4env_err.a
TARGET_amd64	  = libl4env_err.a
BUILD_PIC	= libl4env_loaderruntime.a libl4env_err.a
MODE 		= l4env
SYSTEMS		= x86-l4v2 x86-l4x2 x86 arm-l4x0 arm amd64-l4v2

vpath %.S $(SRC_DIR)/ARCH-$(ARCH)

PRIVATE_INCDIR	= $(SRC_DIR)/../include

# error library
SRC_C_libl4env_err.a	= errstr.c
SRC_C_libl4env_err.p.a	= errstr.c
SRC_C_libl4env_err.pr.a	= errstr.c

# for L4Env application
SRC_C_libl4env.a	= startup.c environment.c errstr-l4ipc.c \
			  errstr-events.c
SRC_C_libl4env.p.a	= $(SRC_C_libl4env.a)
SRC_C_libl4env.pr.a	= $(SRC_C_libl4env.a)

# for including in loader runtime library
SRC_C_libl4env_loaderruntime.p.a  = environment.c
SRC_C_libl4env_loaderruntime.pr.a = environment.c

include $(L4DIR)/mk/lib.mk
