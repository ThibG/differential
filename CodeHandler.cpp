#include "CodeHandler.h"
#include <clang/AST/ASTConsumer.h>
#include <llvm/Support/Host.h>
extern llvm::cl::list<std::string> IgnoredParams;
extern llvm::cl::list<std::string> DefinedMacros;
extern llvm::cl::list<std::string> IncludeDirs;

namespace differential {

// Append a #define line to Buf for Macro.  Macro should be of the form XXX,
// in which case we emit "#define XXX 1" or "XXX=Y z W" in which case we emit
// "#define XXX Y z W".  To get a #define with no value, use "XXX=".
void CodeHandler::DefineBuiltinMacro(vector<char> &Buf, const char *Macro, const char *Command) {
	Buf.insert(Buf.end(), Command, Command+strlen(Command));
	if ( const char *Equal = strchr(Macro, '=') ) {
		// Turn the = into ' '.
		Buf.insert(Buf.end(), Macro, Equal);
		Buf.push_back(' ');
		Buf.insert(Buf.end(), Equal+1, Equal+strlen(Equal));
	} else {
		// Push "macroname 1".
		Buf.insert(Buf.end(), Macro, Macro+strlen(Macro));
		Buf.push_back(' ');
		Buf.push_back('1');
	}
	Buf.push_back('\n');
}

CodeHandler::CodeHandler(string filename)  :
    				file_manager_(FileSystemOptions()),
    				header_search_(file_manager_),
    				diagnostics_engine_(llvm::IntrusiveRefCntPtr<DiagnosticIDs>(new DiagnosticIDs())),
    				source_manager_(diagnostics_engine_,file_manager_),
    				text_diag_printer_(new TextDiagnosticPrinter(llvm::errs(), diagnostic_options_)),
    				contex_ptr(0)
{
	const FileEntry *file_entry_ptr = file_manager_.getFile(filename);
	if ( !file_entry_ptr ) {
		cerr << "Failed to open \'" << filename << "\'" << endl;
		exit(1);
	}

	diagnostics_engine_.setClient(text_diag_printer_);
	diagnostics_engine_.setSourceManager(&source_manager_);

	// Allow C99, C++ and GNU extenstions
	language_options_.C99 = 1;
	language_options_.GNUKeywords = 1;
	language_options_.Borland = 1;
	language_options_.CPlusPlus = 1;
	language_options_.CPlusPlus0x = 1;
	language_options_.Bool = 1;
	language_options_.ParseUnknownAnytype = 1;

	// Setting the target machine properties
	target_options_.Triple = llvm::sys::getHostTriple();

	// Create a target information object
	target_info_ = TargetInfo::CreateTargetInfo(diagnostics_engine_, target_options_);

	header_search_options_.UseStandardSystemIncludes = true;
	header_search_options_.UseBuiltinIncludes = true;
	header_search_options_.UseStandardCXXIncludes = true;
	header_search_options_.UseLibcxx = true;
	header_search_options_.Verbose = 1;

	/*
	header_search_options_.ResourceDir = "/usr/local/lib/clang/" CLANG_VERSION_STRING;
	header_search_options_.AddPath("/usr/local/lib/clang/" CLANG_VERSION_STRING "/include",frontend::Angled, true, false, true);
	header_search_options_.AddPath("/usr/include",frontend::Angled, true, false, true);
	header_search_options_.AddPath("/usr/local/include",frontend::Angled, true, false, true);
	header_search_options_.AddPath("/usr/include/linux",frontend::Angled, true, false, true);
	header_search_options_.AddPath("/usr/include/c++/4.8",frontend::Angled, true, false, true);
	header_search_options_.AddPath("/usr/include/c++/4.8/tr1",frontend::Angled, true, false, true);
	header_search_options_.AddPath("/usr/include/i386-linux-gnu/c++/4.8",frontend::Angled, true, false, true);
	 */

	// Add user header search directories
	for ( unsigned int i = 0;i < IncludeDirs.size();++i ) {
		cerr << "adding " << IncludeDirs[i] << endl;
		header_search_options_.AddPath(IncludeDirs[i], frontend::Angled, true, false, true);
	}
	ApplyHeaderSearchOptions(header_search_, header_search_options_, language_options_, target_info_->getTriple());

	// Add defines passed in through parameters
	vector<char> predefineBuffer;
	for ( unsigned int i = 0;i < DefinedMacros.size();++i ) {
		cerr << "defining " << DefinedMacros[i] << '\n';
		DefineBuiltinMacro(predefineBuffer, DefinedMacros[i].c_str());
	}
	//	DefineBuiltinMacro(predefineBuffer,"_Bool int");
	//	DefineBuiltinMacro(predefineBuffer,"size_t int");
	predefineBuffer.push_back('\0');

	// Create the preproccessor from all the other inputs
	CompilerInstance compiler_instance;
	preprocessor_ptr_ = new Preprocessor(diagnostics_engine_, language_options_, target_info_, source_manager_, header_search_, compiler_instance);
	//	InitializePreprocessor(*preprocessor_ptr_, preprocessor_options_, header_search_options_, frontend_options_);
	preprocessor_ptr_->setPredefines(&predefineBuffer[0]);

	text_diag_printer_->BeginSourceFile(language_options_, preprocessor_ptr_);
	source_manager_.createMainFileID(file_entry_ptr);
}

CodeHandler::~CodeHandler() {
	delete target_info_;
	delete preprocessor_ptr_;
	// TODO: deleting text_diag_printer_ causes a seg fault in the diagnostic engine destructor code
	delete contex_ptr;
}

void CodeHandler::Init(int argc, char *argv[]) {
	llvm::cl::ParseCommandLineOptions(argc, argv, "");
	if ( !IgnoredParams.empty() ) {
		cerr << "Ignoring the following parameters:";
		copy(IgnoredParams.begin(), IgnoredParams.end(), ostream_iterator<string>(cerr, " "));
	}
}

ASTContext * CodeHandler::getAST(){
	if (!contex_ptr) { // create the AST
		IdentifierTable id_table(language_options_);
		SelectorTable selector_table;
		Builtin::Context builtin_contex;
		contex_ptr = new ASTContext(language_options_, source_manager_, target_info_, id_table, selector_table, builtin_contex, 0);
		//AnalysisConsumer consumer(*contex_ptr, diagnostics_engine_, preprocessor_ptr_, cerr, false);
		// ParseAST() requires some consumer to pass the parsed AST to
		class : public ASTConsumer {  } consumer;
		ParseAST(*preprocessor_ptr_, &consumer , *contex_ptr);
	}
	return contex_ptr;
}

}
