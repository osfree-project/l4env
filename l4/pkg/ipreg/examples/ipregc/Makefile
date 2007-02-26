PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= $(PKGNAME)c

# list your .c files here
SRC_C		= main.c l4env_dummies.c

# if your server implements the server side of an idl defined in an idl-file
# of your package, list the idl file name(s) here (no path needed)
CLIENTIDL	= ipreg.idl

MODE		= l4linux
# list additional library paths and libraries here
LIBS		= -llogserver_capsule

include $(L4DIR)/mk/prog.mk
