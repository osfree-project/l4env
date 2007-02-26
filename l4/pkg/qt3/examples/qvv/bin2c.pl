#!/usr/bin/perl
#
# (c) Vladi Belperchinov-Shabanski "Cade" 1999-2002
# http://soul.datamax.bg/~cade  <cade@biscom.net>  <cade@datamax.bg>
#
# SEE `README',`LICENSE' OR `COPYING' FILE FOR LICENSE AND OTHER DETAILS!
#
# $Id$
#

print "static char data[] = { \n";
while(<>)
  {
  for( split // )
    {
    my $o = ord($_);
    print "   $o,\n";
    }
  }
print "0 }; \n";
  