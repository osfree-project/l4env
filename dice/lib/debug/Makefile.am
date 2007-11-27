pkglib_LTLIBRARIES = libdice-debug.la

AM_CPPFLAGS = -I$(top_srcdir)/include

libdice_debug_la_SOURCES = init.cc BETrace.cpp L4BETrace.cpp L4V4BETrace.cpp
noinst_HEADERS = BETrace.h L4BETrace.h L4V4BETrace.h

libdice_debug_la_LDFLAGS = -module -version-number 0:3:0
