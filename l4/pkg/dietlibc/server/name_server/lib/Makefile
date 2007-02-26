PKGDIR?= ../../..
L4DIR ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2 x86-l4x0 #arm-l4x0

# the name of your library
TARGET		= libbasic_name_server.a    \
		  libname_space_provider.a  \
		  libattachable.a           \
		  libname_server.a

# list your .c files here
SRC_C_libbasic_name_server.a	= basic_name_server.c
SRC_C_libname_space_provider.a	= name_space_provider.c
SRC_C_libattachable.a		= attachable.c
SRC_C_libname_server.a		= name_server.c

# workaround for bug in BID
#PRIVATE_INCDIR	= $(PKGDIR)/idl/OBJ-$(SYSTEM)

#LIBCINCDIR	= -I$(GCCINCDIR)
#CPPFLAGS	= -nostdinc
MODE		= l4env_dietlibc_minimal
CLIENTIDL_libbasic_name_server.a	= basic_name_server.idl
CLIENTIDL_libname_space_provider.a	= name_space_provider.idl
CLIENTIDL_libattachable.a		= attachable.idl

include $(L4DIR)/mk/lib.mk
