## Process this file with automake to produce Makefile.in

SUBDIRS = ia32 amd64
SUFFIXES = .cpp
noinst_LIBRARIES = libv2.a
			
libv2_a_SOURCES = L4V2BEClassFactory.cpp L4V2BESizes.cpp L4V2BEIPC.cpp L4V2BEMsgBuffer.cpp L4V2BEDispatchFunction.cpp L4V2BENameFactory.cpp

noinst_HEADERS = L4V2BEClassFactory.h L4V2BESizes.h L4V2BEIPC.h L4V2BEMsgBuffer.h L4V2BEDispatchFunction.h L4V2BENameFactory.h

AM_CXXFLAGS = -Wunused -Wunused-parameter -Wall -W \
	      -Wcast-align -Wcast-qual -Wconversion -Wformat=2 \
	      -Wmissing-format-attribute -Wpointer-arith -Wshadow -Wwrite-strings \
	      -Wstrict-aliasing=2
