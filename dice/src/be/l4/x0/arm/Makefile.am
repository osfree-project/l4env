## Process this file with automake to produce Makefile.in

SUFFIXES		= .cpp
noinst_LIBRARIES	= libx0arm.a
libx0arm_a_SOURCES 	= X0ArmIPC.cpp \
			  X0ArmClassFactory.cpp
noinst_HEADERS		= X0ArmIPC.h \
			  X0ArmClassFactory.h 

AM_CXXFLAGS = -Wunused -Wunused-parameter -Wall -W \
	      -Wcast-align -Wcast-qual -Wconversion -Wformat=2 \
	      -Wmissing-format-attribute -Wpointer-arith -Wshadow -Wwrite-strings
