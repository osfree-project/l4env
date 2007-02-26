#!/usr/bin/perl

# This script preprocesses a LaTeX file to be compiled with latex2html.
# latex2html lacks support of some packets, especially fancyverb. Here we
# do the necessary tranlation.
# Note: This file is not a general solution, it is highly specific for the
#       .tex-files it comes with.

use strict;
my $v;

print "%\n% Don't edit this file. It was generated automatically.\n%\n\n";
while(<>){
    if(s/^\|(.*)\|(\\\\)?$/\1/) {
	$v.=$_
    } else {
	s/\«/\\begin{code}/g;
	s/\»/\\end{code}{}/g;
	s/§/\\\$/g;
	s/\\\#(.*\%.*)?$/\\\\\\hline/g;
	if($v){
	    print "\\begin{rawhtml}<font color=\"#990000\">\\end{rawhtml}\n";
	    print "\\begin{verbatim}\n".$v."\\end{verbatim}\n";
	    print "\\begin{rawhtml}</font>\\end{rawhtml}\n";
	    $v=undef;
	}
	if(/\\examplefile{(.*)}{(.*)}/){
	    print "\\begin{rawhtml}",
	      "<table width=\"100\\%\" bgcolor=\"#EEEEEE\" border=\"2\">",
		"<tr><td align=\"center\">$1</td></tr><tr><td>",
		  "\\end{rawhtml}\n";
	    print "\\begin{verbatim}\n";
	    open(X,"<$2.tex") || die "$2.tex: $!"; while(<X>){print};
	    print "\\end{verbatim}\n";
	    print "\\begin{rawhtml}</td></tr></table>\\end{rawhtml}\n";
	}
	print;
    }
}
