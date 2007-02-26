
INTERFACE:

EXTENSION class Jdb_screen
{
public:
  static const unsigned Mword_size_bmode = 8;
  static const unsigned Mword_size_cmode = 4;
  static const unsigned Col_head_size = 8;
  static const unsigned Columns = 9;
  static const unsigned Rows_total = ((unsigned)-1)/
    				     ((Jdb_screen::Columns-1)*4) + 1;

  static const char*	Mword_zero_dmode;
  static const char*	Mword_invalid_dmode;
  static const char*	Mword_adapter_cmode;
  static const char*	Mword_adapter_bmode;
  static const char*	Mword_not_mapped_cmode;
  static const char*	Mword_not_mapped_bmode;
  static const char*	Mword_blank;

  static const char* const	Reg_names[];
  static const char 		Reg_prefix;
  static const char*		Line;
  
  static const char* Root_page_table;
};

IMPLEMENTATION [ia32,ux,arm]:

const char* Jdb_screen::Mword_zero_dmode	= "       0";
const char* Jdb_screen::Mword_invalid_dmode	= "      -1";
const char* Jdb_screen::Mword_adapter_cmode	= "----";
const char* Jdb_screen::Mword_adapter_bmode	= "--------";
const char* Jdb_screen::Mword_not_mapped_cmode	= "....";
const char* Jdb_screen::Mword_not_mapped_bmode	= "........";
const char* Jdb_screen::Mword_blank		= "        ";

IMPLEMENTATION [ia32,ux]:

const char* const Jdb_screen::Reg_names[] 	= { "EAX", "EBX", "ECX", "EDX", 
						    "EBP", "ESI", "EDI", "EIP", 
						    "ESP", "EFL" };
const char Jdb_screen::Reg_prefix 		= 'E';
const char* Jdb_screen::Line 			= "---------------------------"
						  "---------------------------"
						  "---";

const char* Jdb_screen::Root_page_table		= "pdir: ";
