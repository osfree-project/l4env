## Process this file with automake to produce Makefile.in

SUBDIRS=		idl c-c++

noinst_LIBRARIES= libparser.a
libparser_a_SOURCES= Converter.cpp Preprocessor.cpp SymbolTable.cpp parser-driver.cc
noinst_HEADERS= Converter.h Preprocessor.h SymbolTable.h parser-driver.hh

AM_CPPFLAGS=           -I$(top_builddir)/src
AM_CXXFLAGS = @DICE_CXXFLAGS@

