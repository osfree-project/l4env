SYSTEMS	:= x86-l4v2
PKGDIR	?= ../..
L4DIR	?= $(PKGDIR)/../..

# include configuration
include $(L4DIR)/mk/Makeconf
-include $(PKGDIR_OBJ)/Makeconf.bid.local

ifeq ($(H264_SLICE_SCHEDULE),y)

TARGET	 = libh264sched.a

PRIVATE_INCDIR = \
	$(PKGDIR)/common \
	$(PKGDIR)/lib/video/libavcodec \
	$(PKGDIR)/lib/support/libavutil

CPPFLAGS += -DVERNER
SRC_C	 = llsp.c metrics.c nalu.c process.c propagate.c replace.c schedule.c ssim.c

endif

include $(L4DIR)/mk/lib.mk
