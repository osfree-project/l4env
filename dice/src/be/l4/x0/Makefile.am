## Process this file with automake to produce Makefile.in

SUFFIXES		= .cpp
noinst_LIBRARIES	= libx0.a
libx0_a_SOURCES 	= L4X0BEClassFactory.cpp L4X0BESizes.cpp L4X0BEIPC.cpp L4X0BETrace.cpp L4X0BEMsgBuffer.cpp L4X0BEDispatchFunction.cpp
noinst_HEADERS		= L4X0BEClassFactory.h L4X0BESizes.h L4X0BEIPC.h L4X0BETrace.h L4X0BEMsgBuffer.h L4X0BEDispatchFunction.h

AM_CXXFLAGS = -Wunused -Wunused-parameter -Wall -W \
	      -Wcast-align -Wcast-qual -Wconversion -Wformat=2 \
	      -Wmissing-format-attribute -Wpointer-arith -Wshadow -Wwrite-strings
