PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

PRIVATE_INCDIR	= $(L4DIR)/include/qt3

TARGET		= libqt3_thread.a 

CXXFLAGS	= -fno-rtti -fno-exceptions
DEFINES		= -DQWS -DQ_OS_DROPS -DQT_THREAD_SUPPORT

MODE		= l4env_base
SYSTEMS		= x86-l4v2

#DEBUG		= 1

SRC_CC_libqt3_thread.a	= qmutex_drops.cc qwaitcondition_drops.cc \
			  qthread_drops.cc

include $(L4DIR)/mk/lib.mk
