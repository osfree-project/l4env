#include "c-parser-driver.hh"
#include "c-parser.tab.hh"

#include "fe/FEFile.h"
#include "Compiler.h"
#include <cassert>
#include <fstream>
#include <cstdio>

c_parser_driver::c_parser_driver ()
{ }

c_parser_driver::~c_parser_driver ()
{ }

/** \brief test if the file name looks like a header file
 *  \param f the file name to test
 *  \return true if it has the extension ".h" or ".hh"
 */
bool c_parser_driver::is_header (const std::string& f)
{
	string ext = f.substr(f.rfind(".")+1);
	std::transform(ext.begin(), ext.end(), ext.begin(), _tolower);
	if (ext == string("h") ||
		ext == string("hh"))
		return true;
	return false;
}

#if 0
#include "fe/FEOperation.h"
#include <typeinfo>

static std::string context_to_str(CFEBase *context)
{
	std::string ret("other");
	if (dynamic_cast<CFEFile*>(context))
		ret = "file " + static_cast<CFEFile*>(context)->GetFileName();
	else if (dynamic_cast<CFELibrary*>(context))
		ret = "lib " + static_cast<CFELibrary*>(context)->GetName();
	else if (dynamic_cast<CFEInterface*>(context))
		ret = "interface " + static_cast<CFEInterface*>(context)->GetName();
	else if (dynamic_cast<CFEOperation*>(context))
		ret = "operation " + static_cast<CFEOperation*>(context)->GetName();
	else if (dynamic_cast<CFEStructType*>(context))
		ret = "struct " + static_cast<CFEStructType*>(context)->GetTag();
	else if (dynamic_cast<CFEUnionType*>(context))
		ret = "union " + static_cast<CFEUnionType*>(context)->GetTag();
	else if (!context)
		ret = "(null)";
	else
		ret = typeid(*context).name();
	return ret;
}
#endif

/** \brief setup of include next wrapper
 *  \param f the file to include
 *  \return the file name of the wrapper file
 *
 * The Gnu C and C++ Compiler know the feature include_next. This allows to
 * have multiple versions of header files with the same name in different
 * include directories. When include_next is used the file with the same name
 * is searched not in the current directory but the search starts with the
 * include directory after the include directory where the currently open file
 * was found. Example: there are two header files test.h. One in the directory
 * my_inc_dirs/ and the other in my_inc_dirs/special/. The special directory
 * contains a specialized version of test.h. In this file the directive
 * include_next "test.h" appears, which tells the compiler to search for the
 * next test.h. When arranging the include directories such that the most
 * specialized one comes first, the generic test.h will be included by the
 * most specialized test.h.
 *
 * Because C header files can only be imported into IDL files (not included)
 * and Dice calls the pre-processor on all imported files, it may happen that
 * a file containing a include_next statement is directly fed into the
 * pre-processor. The pre-processor will then issue a warning that it found an
 * include_next statement in a top-level file.
 *
 * To avoid this situation we wrap imported header files into temporary files.
 * We create a temporary file which only contains an include statement with
 * the original file name. This wrapper file is then pre-processed.
 *
 * This approach requires some fixup later, as the parser adds a file object
 * for the wrapper file in the AST.
 */
std::string c_parser_driver::include_next_fix_begin(const std::string& f)
{
	string name;
	if (pCurrentFile && is_header(f) &&
		(determine_filetype(f) != determine_filetype(pCurrentFile->GetFileName())) )
	{
		char *s = new char[sizeof("_wrap_XXXXXX")];
		strcpy(s, "_wrap_XXXXXX");
		int fd = mkstemp(s);
		name = s;
		if (trace_scanning)
			std::cerr << __func__ << "(" << f << ") creates wrapper with name " << s << std::endl;
		write(fd, "#include \"", 10);
		write(fd, f.c_str(), f.length());
		write(fd, "\"\n", 2);
		close(fd);
		delete[] s;
	}
	else
		name = f;
	return name;
}

/** \brief cleanup of include next wrapper
 *  \param f the file name of the wrapper file
 *
 * Removes the wrapper file. For more info on the include next wrapper \see
 * include_next_fix_begin.
 */
void c_parser_driver::include_next_fix_end(const std::string& f)
{
	std::string name = current_file;
	if (name == f)
		return;

	// the current file contains the full file name (including the path):
	// we use it to extract the path used to include the file
	//     string sFull = pCurrentFile->GetFullFileName();
	//     // get path
	//     string sPath = sFull.substr(0, sFull.rfind(pCurrentFile->GetFileName()));
	//     if (trace_scanning)
	// 	std::cerr << __func__ << " current full " << sFull << " makes path " << sPath << std::endl;
	//     // set path of wrapper file
	//     pCurrentFile->SetPath(sPath);
	// remove the wrapper file if !keep-temp-files
	// remove it here, because scan_end has to close it first.
	if (!CCompiler::IsOptionSet(PROGRAM_KEEP_TMP_FILES))
		std::remove( name.c_str() );
}

/** \brief fixup after leaving wrapper file
 *  \param f the file name of the original file
 *  \param pWrapper a reference to the file object of the wrapper file
 *
 * The pre-processor may omit line directives indicating that a wrapper file
 * or included file is left. To fix this, we leave the current context until
 * we reach the wrapper file.
 *
 * For more info on include next wrapper files \see include_next_fix_begin.
 */
void c_parser_driver::include_next_fix_after_parse(const std::string& f, CFEFile *pWrapper)
{
	if (current_file == f)
		return;

	if (trace_scanning)
		std::cerr << __func__ << "(" << f << ", " << (pWrapper ? pWrapper->GetFileName() : "(null)") <<
			") called" << std::endl;

	bool needs_path = pWrapper->GetFileName() == pWrapper->GetFullFileName();
	do
	{
		// test if the path of the wrapper file is set and if not set it to
		// the path of the current file. Do this on each iteration, so only
		// the last (and directly included) file prevails.
		if (needs_path)
		{
			std::string full = pCurrentFile->GetFullFileName();
			std::string path = full.substr(0, full.rfind(pWrapper->GetFileName()));
			pWrapper->SetPath(path);
		}

		leave_file(f, true);
		// now check if we really reached the CFEFile of the wrapper file (the
		// wrapper file and the original file have the same file-name)
	} while (pCurrentFile->GetFileName() == pWrapper->GetFileName() &&
		pCurrentFile != pWrapper);

	if (trace_scanning)
		std::cerr << __func__ << " return current " << pCurrentFile->GetFileName() << " @ " <<
			pCurrentFile << " with wrapper " << pWrapper->GetFileName() << " @ " << pWrapper << std::endl;
}

/** \brief the parse function
 *  \param f the name of the file to parse
 *  \param bPreOnly true if we stop after pre-processing
 *  \param std_inc true if the file was included as standard include file
 *  \return a reference to the created file object
 */
CFEFile* c_parser_driver::parse (const std::string &f, bool bPreOnly, bool std_inc)
{
	// if we start a new C-parser (pCurrentFile has different file-type) and
	// the new file is a header file, we have to create a temporary wrapper
	// file, which includes the header file. The reason is, that if the header
	// file (f) contains an #include_next directive, the compiler may include
	// the wrong files.  Therefore, we pack it all into a wrapper file.
	file = current_file = include_next_fix_begin(f);
	trace_scanning = CCompiler::IsVerboseLevel(PROGRAM_VERBOSE_SCANNER);
	std::string surrounding_file = pCurrentFile ? pCurrentFile->GetFileName() : "";
	scan_begin ();
	if (bPreOnly)
	{
		scan_end();
		return (CFEFile*)0;
	}
	enter_file(f, 1, std_inc, 0, false, current_file == f); // sets pCurrentFile
	CFEFile *pWrapper = pCurrentFile;
	yy::c_parser parser (*this);
	parser.set_debug_level (CCompiler::IsVerboseLevel(PROGRAM_VERBOSE_PARSER));
	parser.parse ();
	// if we had a wrapper file, leave that wrapper file
	include_next_fix_after_parse(f, pWrapper);
	// previous file is current context
	CFEFile *ret = pCurrentFile;
	assert(getCurrentContext() == pCurrentFile);
	// leave file (does not leave if no parent file)
	leave_file(surrounding_file);
	scan_end ();
	// if we used a wrapper file we have to leave it explicetly
	include_next_fix_end(f);
	return ret;
}

