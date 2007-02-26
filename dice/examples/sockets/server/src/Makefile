PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../../../..

TARGET		= $(PKGNAME)-server
# comment the followin two rulesif building for L4
MODE		= host
SYSTEMS		= x86
DEFINES		= -DSOCKETAPI
# and uncomment these
#DEFAULT_RELOC	= 0x00980000

SERVERIDL	= test.idl
SRC_C		= main.c server.c

PRIVATE_INCDIR	= $(L4DIR)/tool/dice/include

# do not install binary
NOTARGETSTOINSTALL=1

include $(L4DIR)/mk/prog.mk
