# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c basic_io.c resolve.c state.c
#PRIVATE_INCDIR = ../../include
SERVERIDL	= object_server.idl \
		  basic_name_server.idl basic_io.idl rw.idl

# target
TARGET	=	static_file_server
SYSTEMS	=	x86-l4v2
MODE	=	l4env_dietlibc_minimal
DEFAULT_RELOC   = 0x01100000
LIBS	+=	hosts.o -lname_server -lname_space_provider

#DEBUG		= 1

include $(L4DIR)/mk/prog.mk

static_file_server: hosts

hosts: hosts
	objcopy -Ibinary -Oelf32-little ../hosts hosts.o

# we do not need other L4 libs than this
#LIBCLIBS_x86_l4linux = -lnames -lrmgr
