PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
SYSTEMS         := x86-l4v2 arm-l4v2 amd64-l4v2
TARGET		= libl4sys.a libl4sys-l4x.a
BUILD_PIC	= $(TARGET)

ifeq ($(ARCH),arm)

SRC_C		= utcb.c
SRC_S		:= ARCH-arm/atomic_ops_s.S

else

SRC_C                   = ipc.c syscalls.c syscalls_indirect.c
SRC_C_libl4sys.a        = utcb.c
SRC_C_libl4sys-l4x.a    = utcb-l4x.c
SRC_S_libl4sys.a       += syscalls_direct.S
SRC_S_libl4sys-l4x.a   += syscalls_direct.S
SRC_C_libl4sys.p.a     += utcb.c
SRC_C_libl4sys-l4x.p.a += utcb-l4x.c
vpath syscalls_direct.S $(SRC_DIR)/ARCH-$(ARCH)

ifeq ($(ARCH),x86)
vpath %.S $(SRC_DIR)/ARCH-x86
vpath syscall_fixup.c $(SRC_DIR)/ARCH-x86
SRC_C_libl4sys.p.a     += syscall_fixup.c
SRC_C_libl4sys-l4x.p.a += syscall_fixup.c
SRC_S_libl4sys.p.a += syscall_ipc_fixup.S \
		      syscall_id_nearest_fixup.S \
		      syscall_fpage_unmap_fixup.S \
		      syscall_thread_switch_fixup.S \
		      syscall_thread_schedule_fixup.S \
		      syscall_lthread_ex_regs_fixup.S \
		      syscall_task_new_fixup.S \
		      syscall_privctrl_fixup.S
SRC_S_libl4sys-l4x.p.a = $(SRC_S_libl4sys.p.a)

endif
endif

include $(L4DIR)/mk/lib.mk
