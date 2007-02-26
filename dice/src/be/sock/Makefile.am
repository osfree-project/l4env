## Process this file wit automake to produce Makefile.in

noinst_LIBRARIES	= libsock.a
libsock_a_SOURCES = SockBEClassFactory.cpp SockBECallFunction.cpp SockBEWaitAnyFunction.cpp SockBESrvLoopFunction.cpp SockBEUnmarshalFunction.cpp SockBESizes.cpp BESocket.cpp SockBEMarshalFunction.cpp SockBESndFunction.cpp

noinst_HEADERS = SockBEClassFactory.h SockBECallFunction.h SockBEWaitAnyFunction.h SockBESrvLoopFunction.h SockBEUnmarshalFunction.h SockBESizes.h BESocket.h SockBEMarshalFunction.h SockBESndFunction.h

AM_CXXFLAGS = -Wunused -Wunused-parameter -Wall -W \
	      -Wcast-align -Wcast-qual -Wconversion -Wformat=2 \
	      -Wmissing-format-attribute -Wpointer-arith -Wshadow -Wwrite-strings
