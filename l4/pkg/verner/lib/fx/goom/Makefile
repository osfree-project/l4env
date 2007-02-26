SYSTEMS			:= 	x86-l4v2
PKGDIR			?=	../../..
L4DIR			?=	$(PKGDIR)/../..

PRIVATE_INCDIR		 = 	$(PKGDIR)/lib/include $(PKGDIR)/include $(SDL_INCDIR)
CFLAGS			+=  -O2 -DHAVE_MMX=1 -fno-schedule-insns -DVERSION=\"1.10\"

TARGET			 =	libfxgoom.a

SRC_C          = zoom_filter_xmmx.c surface.c pixeldoubler.c surf3d.c tentacle3d.c \
		 sdl_goom.c filters.c goom_core.c gfontlib.c graphic.c ifs.c \
		 ifs_display.c lines.c zoom_filter_mmx.c 

# include configuration
include $(L4DIR)/mk/Makeconf
-include $(PKGDIR_OBJ)/Makeconf.bid.local

# use petze ?
ifeq ($(GOOM_PETZE),y)
include $(L4DIR)/pkg/petze/mk/petze.mk
endif

# lib 
include $(L4DIR)/mk/lib.mk


