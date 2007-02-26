## Process this file with automake to produce Makefile.in

SUBDIRS = ia32

SUFFIXES = .cpp
noinst_LIBRARIES = libv4.a
libv4_a_SOURCES = L4V4BEClassFactory.cpp L4V4BEIPC.cpp L4V4BENameFactory.cpp L4V4BESizes.cpp L4V4BECallFunction.cpp L4V4BEMarshalFunction.cpp L4V4BEWaitAnyFunction.cpp L4V4BEMsgBuffer.cpp L4V4BETrace.cpp L4V4BEMarshaller.cpp L4V4BESndFunction.cpp L4V4BEWaitFunction.cpp L4V4BESrvLoopFunction.cpp

noinst_HEADERS = L4V4BEClassFactory.h L4V4BEIPC.h L4V4BENameFactory.h L4V4BESizes.h L4V4BECallFunction.h L4V4BEMarshalFunction.h L4V4BEWaitAnyFunction.h L4V4BEMsgBuffer.h L4V4BETrace.h L4V4BEMarshaller.h L4V4BESndFunction.h L4V4BEWaitFunction.h L4V4BESrvLoopFunction.h

AM_CXXFLAGS = -Wunused -Wunused-parameter -Wall -W \
	      -Wcast-align -Wcast-qual -Wconversion -Wformat=2 \
	      -Wmissing-format-attribute -Wpointer-arith -Wshadow -Wwrite-strings
