PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= $(PKGNAME)_server
# the default relocation address. This may be superseded by a STATIC file.
DEFAULT_RELOC	= 0x01800000

# list your .c files here
SRC_C		= main.c signal_components.c helper.c manage_data.c \
				send_signal.c alarm_sched.c

# if your server implements the server side of an idl defined in an idl-file
# of your package, list the idl file name(s) here (no path needed)
SERVERIDL	= signal.idl

# list additional library paths and libraries here
LIBS		= -lsignal

MODE		= l4env_minimal

include $(L4DIR)/mk/prog.mk
