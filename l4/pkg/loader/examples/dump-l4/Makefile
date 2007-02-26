PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
TARGET		= dump-l4
SRC_C 		= main.c l4env_dummies.c
MODE		= l4linux
LIBS		= -ldm_generic -llogserver_capsule -ll4sys
SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0

include $(L4DIR)/mk/prog.mk
