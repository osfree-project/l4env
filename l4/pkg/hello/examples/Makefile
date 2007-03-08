PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..
TARGET		= many_mappings many_tasks many_threads arm_test schedtest \
		  abs_timeout top ioflexpage next_res fast_thread_time \
		  tlbcheck

# Note, do not add rt_sched to TARGET, it has dependencies which cannot be
#       fulfilled in any case (e.g. hello build)

include $(L4DIR)/mk/subdir.mk
