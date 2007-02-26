PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../../../..

IDL		= test.idl
# comment the following rules if building for L4
L4API		= linux
SYSTEMS 	= x86
#IDL_FLAGS	= -v7

NOTARGETSTOINSTALL=1
# uncomment following rule if building for L4
#SYSTEMS=x86-l4v2 x86-l4x0

include $(L4DIR)/mk/idl.mk
