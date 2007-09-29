#
# $Id: Makefile 28603 2007-05-24 08:28:33Z aw11 $
#
# Makefile for the demo of the names server
#

SYSTEMS := x86-l4v2 
PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SRC_C			= bmsi_test.c
#MODE			= l4env
#MODE			= sigma0

TARGET			= bmsi_test
DEFAULT_RELOC_x86	= 0x00e00000

LIBS			= -lbmsi_builder -lbmsi_integrity \
			  -llyon-client -ltcg -ltcg_crypt -lcrypto_sha1_openssl

include $(L4DIR)/mk/prog.mk
