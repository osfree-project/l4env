PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

# files
SRC_S		= crt0_getopt.S  crt0_l4env_dyn.S  crt0_l4env_noenv.S \
		  crt0_l4env_stat.S crt0_oskit10_support.S crt0_l4env.S \
		  crt0_l4env_freebsd.S crtn.S
SYSTEMS		= x86-l4v2 x86-l4x0

INSTALL_TARGET	= $(OBJS) main_stat.ld
OBJS		= $(SRC_S:.S=.o)

include $(L4DIR)/mk/lib.mk
