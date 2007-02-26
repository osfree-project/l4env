#ifndef __PARSE_CMD_H
#define __PARSE_CMD_H

enum parse_cmd_type{
    PARSE_CMD_INT,
    PARSE_CMD_SWITCH,
    PARSE_CMD_STRING,
};

/* Parse the command-line for specified arguments and store the values into
   variables.

   This Functions gets the command-line, and a list of
   command-descriptors. Then, the command-line is checked against the
   given descriptors, storing strings, switches and numeric arguments
   at given addresses. A default help descriptor is added. Its purpose
   is to present a short command overview in the case the given
   command-line does not fit to the descriptors.

   Each command-descriptor has the following form:

   short option char, long option name, comment, type, val, addr.

   The short option char specifies the short form of the described
   option. The short form will be recognized after a single dash, or
   in a group of short options preceeded by a single dash. Specify ' '
   if no short form should be used.

   The long option name specifies the long form of the described
   option. The long form will be recognized after two dashes. Specify
   0 if no long form should be used for this option.

   The comment is a string that will be used when presenting the short
   command-line help.

   The type specifies, if the option should be recognized as a number
   (PARSE_CMD_INT), a switch (PARSE_CMD_SWITCH) or a string
   (PARSE_CMD_STRING). If PARSE_CMD_INT, the option requires a second
   argument on the command-line after the option. This argument is
   parsed as a number. It can be preceeded by 0x to present a
   hex-value or by 0 to present an octal form. If the type is
   PARSE_CMD_SWITCH, no additional argument is expected at the
   command-line. If the type is PARSE_CMD_STRING, an additional string
   argument is expected.
   
   The <addr> specifies, how recognized options should be returned. If
   <type> is PARSE_CMD_INT, <addr> is a pointer to an int. The scanned
   argument from the command-line is stored in this pointer. If <type>
   is PARSE_CMD_SWITCH, <addr> must be a pointer to int, and the value
   from <val> is stored at this pointer. If <type> is
   PARSE_CMD_STRING, <addr> must be a pointer to const char*, and a
   pointer to the argument on the command line is stored at this
   pointer.

   For the types PARSE_CMD_INT and PARSE_CMD_STRING, the value in
   <val> is a default value, which is stored in the given pointers if
   the corresponding option is not given on the command line. If type
   is PARSE_CMD_SWITCH, and the corresponding options are not given on
   the command-line, the value <addr> points to is not modified.

   The list of command-descriptors is terminated by specifying a
   binary 0 for the short option char.

   Note: The short option char 'h' and the long option name "help"
   must not be specified. They are used for the default help
   descriptor and produce a short command-options help when specified
   on the command-line.

   Return value:

   The function returns 0 if the command-line was successfully parsed.
   The function returns -1 if the given descriptors are somehow wrong.
   The function returns -2 if not enough memory was available to hold
   temporary structs.
   The function returns -3 if the given command-line args did not meet
   the specified set.
   The function returns -4 if the help-option was given.

   Upon return, argc and argv point to a list of arguments that were
   not scanned as arguments. See getoptlong for details on scanning.

*/
int parse_cmdline(int *argc, const char***argv, char arg0,...);

#endif

