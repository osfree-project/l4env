## Process this file with automake to produce Makefile.in

noinst_LIBRARIES =	libcdr.a
libcdr_a_SOURCES = CCDRClassFactory.cpp \
			CCDRClient.cpp \
			CCDRClass.cpp \
			CCDRComponentFunction.cpp \
			CCDRMarshalFunction.cpp \
			CCDRUnmarshalFunction.cpp

noinst_HEADERS = CCDRClassFactory.h \
			CCDRClient.h \
			CCDRClass.h \
			CCDRComponentFunction.h \
			CCDRMarshalFunction.h \
			CCDRUnmarshalFunction.h 

AM_CXXFLAGS = -Wunused -Wunused-parameter -Wall -W \
	      -Wcast-align -Wcast-qual -Wconversion -Wformat=2 \
	      -Wmissing-format-attribute -Wpointer-arith -Wshadow -Wwrite-strings
