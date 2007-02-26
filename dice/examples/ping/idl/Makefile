PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../../../..

IDL		= ping.idl
IDL_FLAGS	= -T -fforce-c-bindings -ftestsuite-shutdown -ftest-no-success-message
MODE		= l4env
SYSTEMS		= x86-l4v2 x86-l4x0

include $(L4DIR)/mk/idl.mk
