# directories we need to know
PKGDIR		= ../..
L4DIR		?= $(PKGDIR)/../..

-include $(PKGDIR)/Makeconf.bid.local

# only one XviD version can be used!
ifeq ($(BUILD_xvid10),y)
BUILD_xvid09 = n
endif
ifeq ($(BUILD_xvid09),y)
TARGET += xvidcore-0.9.1
endif
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
TARGET += xvidcore-0.9.1 xvidcore-1.0 libavcodec libmpeg2
endif

# include subdir role
include $(L4DIR)/mk/subdir.mk
