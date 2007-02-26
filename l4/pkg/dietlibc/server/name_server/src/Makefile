# directories we need to know
PKGDIR ?=	../../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =	main.c volumes.c dirs.c mount_table.c resolve.c pathnames.c \
	clientstate.c

#PRIVATE_INCDIR = ../../include

# the last four are a fix for a current idl bug
SERVERIDL =	name_server.idl \
		basic_name_server.idl name_space_provider.idl \
		attachable.idl basic_io.idl rw.idl

# target
TARGET	= name_server
SYSTEMS	= x86-l4v2 x86-l4x0
MODE	= l4env_dietlibc_minimal
#L4LIBS =
LIBS	= -lbasic_name_server

DEFAULT_RELOC	= 0x01000000

#DEBUG		= 1

include $(L4DIR)/mk/prog.mk

# we do not need other L4 libs than this
#LIBCLIBS_x86_l4linux = -lnames -lrmgr
