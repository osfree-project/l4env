SYSTEMS         := x86-l4v2 x86-l4x0
PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= presenter

# the default relocation address. This may be superseded by a STATIC file.
DEFAULT_RELOC	= 0x01800000

# list your .c files here
SRC_C		= main.c slide.c presentation.c presenter.c presmanager.c 

# if your server implements the server side of an idl defined in an idl-file
# of your package, list the idl file name(s) here (no path needed)
#SERVERIDL	= 

DEBUG          = 1

#list additional library paths and libraries here
LIBS		= -lutil -lcontroller -lview -ll4dope -lvscreen -ldm_phys -ll4env -lgeneric_fprov $(GCCLIB)

PRIVATE_INCDIR  = $(PKGDIR)/server/include  

PRIVATE_LIBDIR   = $(PKGDIR)/server/src/util/OBJ-$(SYSTEM) $(PKGDIR)/server/src/controller/OBJ-$(SYSTEM) \
		   $(PKGDIR)/server/src/view/OBJ-$(SYSTEM)

include $(L4DIR)/mk/prog.mk

