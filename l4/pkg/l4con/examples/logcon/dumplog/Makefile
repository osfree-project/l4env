PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..
SYSTEMS		= x86-l4v2 arm-l4v2
TARGET		= dumplog
SRC_C 		= main.c
MODE		= l4linux
IDL_PKGDIR      = $(PKGDIR_OBJ)/examples/logcon
CLIENTIDL       = logcon.idl
LIBS		= -lparsecmdline -ldm_generic -llogserver_capsule

include $(L4DIR)/mk/prog.mk
