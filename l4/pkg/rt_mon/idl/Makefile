PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2

IDL		= rt_mon.idl l4vfs_rt_mon.idl rt_mon_reg.idl

# enable external servers
IDL_EXPORT_SKELETON	= rt_mon.idl l4vfs_rt_mon.idl rt_mon_reg.idl

IDL_FLAGS	= -t -ffree-mem-after-reply # -fforce-corba-alloc -ftrace-dump-msgbuf -ftrace-dump-msgbuf-dwords=4 -ftrace-function=LOG

include $(L4DIR)/mk/idl.mk
