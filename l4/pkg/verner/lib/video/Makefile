# directories we need to know
PKGDIR		= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf
-include $(PKGDIR_OBJ)/Makeconf.bid.local

ifeq ($(BUILD_xvid10),y)
TARGET += xvidcore-1.0
endif
ifeq ($(BUILD_libavcodec),y)
TARGET += libavcodec
endif
ifeq ($(BUILD_libmpeg2),y)
TARGET += libmpeg2
endif

TARGET += postproc

ifneq ($(filter clean cleanall,$(MAKECMDGOALS)),)
TARGET += xvidcore-1.0 libavcodec libmpeg2
endif

# include subdir role
include $(L4DIR)/mk/subdir.mk
