## Process this file with automake to produce Makefile.in

SUFFIXES = .cpp
noinst_LIBRARIES = libv4ia32.a

libv4ia32_a_SOURCES = L4V4IA32IPC.cpp L4V4IA32ClassFactory.cpp 

noinst_HEADERS = L4V4IA32IPC.h L4V4IA32ClassFactory.h

AM_CXXFLAGS = -Wunused -Wunused-parameter -Wall -W \
	      -Wcast-align -Wcast-qual -Wconversion -Wformat=2 \
	      -Wmissing-format-attribute -Wpointer-arith -Wshadow -Wwrite-strings
