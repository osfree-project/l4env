PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= $(PKGNAME)
# the default relocation address. This may be superseded by a STATIC file.
DEFAULT_RELOC	= 0x01700000

PRIVATE_INCDIR	= $(PKGDIR)/server/include

#DEBUG		= 1

# list your .c files here
SRC_C		= main.c server.c socket_internal.c events.c

# if your server implements the server side of an idl defined in an idl-file
# of your package, list the idl file name(s) here (no path needed)
SERVERIDL	= local_socks.idl
CLIENTIDL	= local_socks.idl

# list additional library paths and libraries here
LIBS		= -ll4vfs_network_server-server -ll4vfs_select_notify-server \
		  -ll4vfs_select -levents

DIETLIBC_MALLOC_BACKEND	= -lc_be_buddy_slab_mem
MODE			= l4env_minimal

include $(L4DIR)/mk/prog.mk
