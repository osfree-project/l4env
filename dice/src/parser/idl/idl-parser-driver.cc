#include "idl-parser-driver.hh"
#include "idl-parser.tab.hh"

#include "fe/FEFile.h"
#include "Compiler.h"

#include <cassert>

idl_parser_driver::idl_parser_driver ()
    : expect_attr(false)
{
    typedef yy::idl_parser::token token;
    // intialize symbol table with known attributes
    attr_table.clear();
    attr_table["abs"] = token::ABS;
    attr_table["abstract"] = token::ABSTRACT;
    attr_table["allow_reply_only"] = token::ALLOW_REPLY_ONLY;
    attr_table["auto_handle"] = token::AUTO_HANDLE;
    attr_table["broadcast"] = token::BROADCAST;
    attr_table["callback"] = token::CALLBACK;
    attr_table["context_handle"] = token::CONTEXT_HANDLE;
    attr_table["dedicated_partner"] = token::DEDICATED_PARTNER;
    attr_table["default_function"] = token::DEFAULT_FUNCTION;
    attr_table["default_timeout"] = token::DEFAULT_TIMEOUT;
    attr_table["endpoint"] = token::ENDPOINT;
    attr_table["error_function"] = token::ERROR_FUNCTION;
    attr_table["error_function_client"] = token::ERROR_FUNCTION_CLIENT;
    attr_table["errfunc_client"] = token::ERROR_FUNCTION_CLIENT;
    attr_table["error_function_server"] = token::ERROR_FUNCTION_SERVER;
    attr_table["errfunc_server"] = token::ERROR_FUNCTION_SERVER;
    attr_table["exceptions"] = token::EXCEPTIONS;
    attr_table["first_is"] = token::FIRST_IS;
    attr_table["handle"] = token::HANDLE;
    attr_table["helpcontext"] = token::HELPCONTEXT;
    attr_table["helpfile"] = token::HELPFILE;
    attr_table["helpstring"] = token::HELPSTRING;
    attr_table["hidden"] = token::HIDDEN;
    attr_table["idempotent"] = token::IDEMPOTENT;
    attr_table["ignore"] = token::IGNORE;
    attr_table["iid_is"] = token::IID_IS;
    attr_table["init_rcvstring"] = token::INIT_RCVSTRING;
    attr_table["init_rcvstring_client"] = token::INIT_RCVSTRING_CLIENT;
    attr_table["init_rcvstring_server"] = token::INIT_RCVSTRING_SERVER;
    attr_table["last_is"] = token::LAST_IS;
    attr_table["lcid"] = token::LCID;
    attr_table["length_is"] = token::LENGTH_IS;
    attr_table["local"] = token::LOCAL;
    attr_table["max_is"] = token::MAX_IS;
    attr_table["maybe"] = token::MAYBE;
    attr_table["min_is"] = token::MIN_IS;
    attr_table["noexceptions"] = token::NOEXCEPTIONS;
    attr_table["noopcode"] = token::NOOPCODE;
    attr_table["object"] = token::OBJECT;
    attr_table["oneway"] = token::ONEWAY;
    attr_table["pointer_default"] = token::POINTER_DEFAULT;
    attr_table["prealloc_client"] = token::PREALLOC_CLIENT;
    attr_table["prealloc_server"] = token::PREALLOC_SERVER;
    attr_table["ptr"] = token::PTR;
    attr_table["ref"] = token::REF;
    attr_table["reflect_deletions"] = token::REFLECT_DELETIONS;
    attr_table["restricted"] = token::RESTRICTED;
    attr_table["sched_donate"] = token::SCHED_DONATE;
    attr_table["schedule_donate"] = token::SCHED_DONATE;
    attr_table["size_is"] = token::SIZE_IS;
    attr_table["string"] = token::STRING;
    attr_table["switch_is"] = token::SWITCH_IS;
    attr_table["switch_type"] = token::SWITCH_TYPE;
    attr_table["transmit_as"] = token::TRANSMIT_AS;
    attr_table["typeof"] = token::TYPEOF;
    attr_table["unique"] = token::UNIQUE;
    attr_table["uuid"] = token::UUID;
    attr_table["version"] = token::VERSION_ATTR;
}

idl_parser_driver::~idl_parser_driver ()
{
}

#if 0
static std::string context_to_str(CFEBase *context)
{
    std::string ret("other");
    if (dynamic_cast<CFEFile*>(context))
	ret = " file " + static_cast<CFEFile*>(context)->GetFileName();
    else if (dynamic_cast<CFELibrary*>(context))
	ret = " lib " + static_cast<CFELibrary*>(context)->GetName();
    else if (dynamic_cast<CFEInterface*>(context))
	ret = " interface " + static_cast<CFEInterface*>(context)->GetName();
    return ret;
}
#endif

CFEFile*
idl_parser_driver::parse (const std::string &f, bool bPreOnly, bool std_inc)
{
    file = current_file = f;
    std::string surrounding_file = pCurrentFile ? pCurrentFile->GetFileName() : "";
    trace_scanning = CCompiler::IsVerboseLevel(PROGRAM_VERBOSE_SCANNER);
    scan_begin ();
    if (bPreOnly)
    {
	scan_end();
	return (CFEFile*)0;
    }
    enter_file(f, 1, std_inc, 0, false, true); // sets pCurrentFile makes it child
    yy::idl_parser parser (*this); // this is the bison generated parser class
    parser.set_debug_level (CCompiler::IsVerboseLevel(PROGRAM_VERBOSE_PARSER));
    parser.parse ();
    // previous file is current context
    CFEFile *ret = pCurrentFile;
    assert(getCurrentContext() == pCurrentFile);
    // leave file (does not leave if no parent file)
    leave_file(surrounding_file);
    scan_end ();
    return ret;
}

idl_parser_driver::tokentype
idl_parser_driver::find_attribute (const std::string& s)
{
    if (!expect_attr)
	return yy::idl_parser::token::INVALID;
    if (attr_table.find(s) == attr_table.end())
	return yy::idl_parser::token::INVALID;
    return attr_table[s];
}
