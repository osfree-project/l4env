INTERFACE[amd64]:

#include "config.h"

EXTENSION class Jdb_screen
{
public:
  static const unsigned Mword_size_bmode = 16;
  static const unsigned Mword_size_cmode = 8;
  
  static const unsigned Col_head_size = 8;
  static const unsigned Columns	= 5;
  static const unsigned Rows_total = Config::SUPERPAGE_SIZE/
    				     ((Jdb_screen::Columns-1) * sizeof(Mword));
  
  static const char*	Mword_zero_dmode;
  static const char*	Mword_invalid_dmode;
  static const char*	Mword_adapter_cmode;
  static const char*	Mword_adapter_bmode;
  static const char*	Mword_not_mapped_cmode;
  static const char*	Mword_not_mapped_bmode;
  static const char*	Mword_blank;

  static const char * const Reg_names[];
  static const char Reg_prefix;
  static const char* Line;

  static const char* Root_page_table;
};

IMPLEMENTATION [amd64]:

const char* Jdb_screen::Mword_zero_dmode	= "               0";
const char* Jdb_screen::Mword_invalid_dmode	= "              -1";
const char* Jdb_screen::Mword_adapter_cmode	= "~~~~~~~~";
const char* Jdb_screen::Mword_adapter_bmode	= "~~~~~~~~~~~~~~~~";
const char* Jdb_screen::Mword_not_mapped_cmode	= "--------";
const char* Jdb_screen::Mword_not_mapped_bmode	= "----------------";
const char* Jdb_screen::Mword_blank		= "                ";


const char * const Jdb_screen::Reg_names[] 	= { "RAX", "RBX", "RCX", "RDX", 						    "RBP", "RSI", "RDI", "R8",
    						    "R9", "R10", "R11", "R12",
						    "R13", "R14", "R15", "RIP",
						    "RSP", "RFL" };
const char  Jdb_screen::Reg_prefix		 = 'R';
const char* Jdb_screen::Line			 = "--------------------------"
						   "----------------------";

const char* Jdb_screen::Root_page_table		 = "pml4: ";
