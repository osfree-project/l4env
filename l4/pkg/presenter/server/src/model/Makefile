SYSTEMS         := x86-l4v2
PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= presenter

# the default relocation address. This may be superseded by a STATIC file.
DEFAULT_RELOC	= 0x01800000

# list your .c files here
SRC_C		= main.c slide.c presentation.c presenter.c presmanager.c

DEBUG           = 0 

include $(L4DIR)/mk/Makeconf

#BUILD_FPROV	= yes

LIBS 		= -lpresenter_controller -lpresenter_view -ll4dope -lvscreen \
		  -ll4png_wrap -ll4png -ll4z -ldm_phys

ifeq ($(BUILD_FPROV),)
LIBS 		+= -lpresenter_util_presprov
else
LIBS		+= -lpresenter_util_fprov -lgeneric_fprov
endif

PRIVATE_INCDIR  = $(PKGDIR_ABS)/server/include  

PRIVATE_LIBDIR   = $(PKGDIR_OBJ)/server/src/util/OBJ-$(SYSTEM) \
		   $(PKGDIR_OBJ)/server/src/controller/OBJ-$(SYSTEM) \
		   $(PKGDIR_OBJ)/server/src/view/OBJ-$(SYSTEM)

include $(L4DIR)/mk/prog.mk

