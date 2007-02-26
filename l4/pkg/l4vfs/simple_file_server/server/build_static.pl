#!/usr/bin/perl -w

$s_file = "s_file = (simple_file_t *) malloc(sizeof(simple_file_t));\n";
$arraylist_add = "arraylist->add_elem(files,s_file);\n";

$state_decl= "state.c.decl";
$state_inc = "state.c.inc";

open(STATE_INC,"> $state_inc");
open(STATE_DECL,"> $state_decl");

while (<>) { 

	$Line = $_; 

	chomp($Line); # kill line feed

	$OrigLine = $Line;

	$Line =~ s/\./_/g;  

	$decl_start = "extern int _binary_".$Line."_start;\n";

	$decl_size = "extern int _binary_".$Line."_size;\n";

	print STATE_DECL "$decl_start";
	print STATE_DECL "$decl_size";

	print STATE_DECL "\n";

	# ok now build state.c.inc

	print STATE_INC "$s_file";

	$s_file_data = "s_file->data = (l4_uint8_t *) &"."_binary_".$Line."_start;\n";

	print STATE_INC "$s_file_data";

	$s_file_length = "s_file->length = (int) &"."_binary_".$Line."_size;\n";

	print STATE_INC "$s_file_length";

	$s_file_name = "s_file->name = strdup(\"".$OrigLine."\");\n";

	print STATE_INC "$s_file_name";

	print STATE_INC "$arraylist_add";

	print STATE_INC "\n";
}

close STATE_INC;
close STATE_DECL;
