## Process this file with automake to produce Makefile.in

SUBDIRS= v2 x0 v4

noinst_LIBRARIES= libl4.a
libl4_a_SOURCES = L4BECallFunction.cpp L4BEHeaderFile.cpp L4BESizes.cpp L4BETypedDeclarator.cpp L4BESndFunction.cpp L4BEUnmarshalFunction.cpp L4BESrvLoopFunction.cpp L4BEWaitAnyFunction.cpp L4BEWaitFunction.cpp L4BEClass.cpp L4BENameFactory.cpp L4BEClassFactory.cpp L4BEReplyFunction.cpp L4BEMarshalFunction.cpp L4BEDispatchFunction.cpp L4BESwitchCase.cpp L4BEMsgBuffer.cpp L4BEMarshaller.cpp L4BEPositionMarshaller.cpp L4BEMsgBufferType.cpp L4BETrace.cpp L4BEIPC.cpp

noinst_HEADERS = L4BECallFunction.h L4BEHeaderFile.h L4BESizes.h L4BETypedDeclarator.h L4BESndFunction.h L4BEUnmarshalFunction.h L4BESrvLoopFunction.h L4BEWaitAnyFunction.h L4BEWaitFunction.h L4BEClass.h L4BENameFactory.h L4BEClassFactory.h L4BEReplyFunction.h L4BEMarshalFunction.h L4BEDispatchFunction.h L4BESwitchCase.h L4BEMsgBuffer.h L4BEMarshaller.h L4BEMsgBufferType.h L4BETrace.h L4BEIPC.h

AM_CXXFLAGS = -Wunused -Wunused-parameter -Wall -W \
	      -Wcast-align -Wcast-qual -Wconversion -Wformat=2 \
	      -Wmissing-format-attribute -Wpointer-arith -Wshadow -Wwrite-strings
