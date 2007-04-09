PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..
TARGET		= dump-l4 fprov-l4 kill-l4 run-l4 run run-kdebug exit

# only build these two examples if we have the ldso
TARGET		+= $(if $(filter y,$(BUILD_LOADER)), shared testapp)

include $(L4DIR)/mk/subdir.mk
