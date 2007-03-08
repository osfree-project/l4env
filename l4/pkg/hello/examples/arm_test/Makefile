PKGDIR			?= ../..
L4DIR			?= $(PKGDIR)/../..

TARGET			 = arm_test
MODE			 = libc
LIBCLIBS		 = -nostdlib $(GCCLIB)
DEFAULT_RELOC_arm_sa	 := 0xc00c0000
DEFAULT_RELOC_arm_pxa	 := 0xa00c0000
DEFAULT_RELOC_arm_isg	 := 0x200c0000
DEFAULT_RELOC		 = $(DEFAULT_RELOC_$(ARCH)_$(CPU))
SYSTEMS			 = arm-l4x0
L4LIBS			+= -lcxx_start -lcxx_thread -lcxx_io -lcxx_io_kdebug \
			   -lcxx_base -lcxx_util
SRC_CC			 = main.cc mem.cc test.cc ipc_test.cc

#CXXFLAGS += -DDO_DEBUG
CXXFLAGS		+= -fno-rtti -fno-exceptions

include $(L4DIR)/mk/prog.mk
