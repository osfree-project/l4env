PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
SYSTEMS         := x86-l4v2 arm-l4v2 amd64-l4v2 x86-l4secv2emu
TARGET		= libl4sys.a
BUILD_PIC	= $(TARGET)

ifeq ($(L4API),l4x2)

SRC_S		= L4API-l4x2/ia32-syscall-stubs.S
SRC_CC		= L4API-l4x2/ia32-syscall-init.cc

else
ifeq ($(L4API),l4secv2emu)
SRC_CC		= L4API-l4secv2emu/wrapper.cc L4API-l4secv2emu/kdebug.cc \
		  L4API-l4secv2emu/ipc.cc \
		  syscall-user.cc l4_types-timeout.cc \
                  l4_types-capability.cc l4_types-fpage-xe.cc l4_types-numbers.cc \
                  l4_types-syscalls.cc syscall_lowlevel.cc debug_glue.cc syscall-user-debug.cc
 
  
VPATH		+= $(FIASCO_DIR)/auto
PRIVATE_INCDIR  += $(FIASCO_DIR)/auto
CXXFLAGS	= -fno-exceptions

else
ifeq ($(ARCH),arm)

SRC_C		=
SRC_S		:= ARCH-arm/atomic_ops_s.S

else

SRC_C		= ipc.c syscalls.c syscalls_indirect.c
SRC_S_libl4sys.a +=   syscalls_direct.S
vpath syscalls_direct.S $(SRC_DIR)/ARCH-$(ARCH)

ifeq ($(ARCH),x86)
vpath %.S $(SRC_DIR)/ARCH-x86
vpath syscall_fixup.c $(SRC_DIR)/ARCH-x86
SRC_C_libl4sys.p.a += syscall_fixup.c
SRC_S_libl4sys.p.a += syscall_ipc_fixup.S \
		      syscall_id_nearest_fixup.S \
		      syscall_fpage_unmap_fixup.S \
		      syscall_thread_switch_fixup.S \
		      syscall_thread_schedule_fixup.S \
		      syscall_lthread_ex_regs_fixup.S \
		      syscall_task_new_fixup.S \
		      syscall_privctrl_fixup.S
endif

endif
endif
endif

include $(L4DIR)/mk/lib.mk
