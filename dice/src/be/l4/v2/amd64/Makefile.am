## Process this file with automake to produce Makefile.in

SUFFIXES = .cpp
noinst_LIBRARIES = libv2amd64.a

libv2amd64_a_SOURCES = V2AMD64ClassFactory.cpp V2AMD64Sizes.cpp V2AMD64NameFactory.cpp

noinst_HEADERS = V2AMD64ClassFactory.h V2AMD64Sizes.h V2AMD64NameFactory.h

AM_CXXFLAGS = -Wunused -Wunused-parameter -Wall -W \
	      -Wcast-align -Wcast-qual -Wconversion -Wformat=2 \
	      -Wmissing-format-attribute -Wpointer-arith -Wshadow -Wwrite-strings \
	      -Wstrict-aliasing=2
