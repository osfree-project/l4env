PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libl4env.a libl4env_noenv.a libl4env_oskit10.a \
		  libl4env_loaderruntime.a
BUILD_PIC	= libl4env_loaderruntime.a
MODE 		= l4env
SYSTEMS		= x86-l4v2 x86-l4x0

PRIVATE_INCDIR	= ../../include

# files
SRC_C			 = environment.c errstr.c
SRC_C_libl4env.a	 = init.c startup.c
SRC_C_libl4env_noenv.a   = init.c startup_noenv.c
SRC_C_libl4env_oskit10.a = init.c startup_oskit10.c

include $(L4DIR)/mk/lib.mk
