PKGDIR	= ..
L4DIR	?= $(PKGDIR)/../..

TARGET	= perf watchdog linuxload rdtsc rdpmc test

# These targets need log, which is not available
# in the hello CVS target -- better solutions welcome
ifneq ($(wildcard $(L4DIR)/pkg/log),)
TARGET	+= list map l4load atomic cmdline spin list_alloc
endif

include $(L4DIR)/mk/subdir.mk
