PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..

TARGET		= demo1 demo2 demo3 evdemo \
		  linux_stub linux_stub_lib xf86_stub logcon \
		  contxtdemo bench dropsshot

include $(L4DIR)/mk/Makeconf

ifneq ($(L4LX24_AVAILABLE),)
TARGET		+= linux24_stub_lib
endif

include $(L4DIR)/mk/subdir.mk
