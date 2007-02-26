PKGDIR		?= ../../
L4DIR		?= $(PKGDIR)/../..

TARGET		= local_socks-test-server local_socks-test-client

# the default relocation address. This may be superseded by a STATIC file.
DEFAULT_RELOC_local_socks-test-server	= 0x01900000
DEFAULT_RELOC_local_socks-test-client	= 0x01950000

# list your .c files here
SRC_C				= util.c
SRC_C_local_socks-test-server	= server.c
SRC_C_local_socks-test-client	= client.c

# if your server implements the server side of an idl defined in an idl-file
# of your package, list the idl file name(s) here (no path needed)
CLIENTIDL	= local_socks.idl

# list additional library paths and libraries here
LIBS		= -lc_be_select -lc_be_socket_io -ll4vfs_select_listener-server \
		  -ll4vfs_net_io -ll4vfs_select

MODE		= l4env_base

include $(L4DIR)/mk/prog.mk
