PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

PRIVATE_INCDIR	= $(L4DIR)/include/qt3

TARGET		= libqt3_qws_client.a libqt3_qws_server.a

CXXFLAGS	= -fno-rtti -fno-exceptions

MODE		= l4env_base
SYSTEMS		= x86-l4v2

#DEBUG		= 1

SERVERIDL_libqt3_qws_server.a	= l4qws_shm.idl l4qws_qlock.idl
CLIENTIDL_libqt3_qws_client.a	= l4qws_shm.idl l4qws_qlock.idl
SRC_C_libqt3_qws_server.a	= l4qws_shm_server.c l4qws_qlock_server.c
SRC_C_libqt3_qws_client.a	= l4qws_shm_client.c l4qws_qlock_client.c \
				  l4qws_key.c
SRC_CC_libqt3_qws_client.a	= qsharedmemory_drops.cc qlock_drops.cc

include $(L4DIR)/mk/lib.mk
