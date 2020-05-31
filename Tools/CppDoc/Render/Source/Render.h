#ifndef VCZH_DOCUMENT_CPPDOC_RENDER
#define VCZH_DOCUMENT_CPPDOC_RENDER

#include <VlppOS.h>
#include <Ast_Resolving.h>
#include <Symbol_TemplateSpec.h>

using namespace vl::stream;
using namespace vl::filesystem;
using namespace vl::console;

/***********************************************************************
Preprocessing
***********************************************************************/

// skip these tokens to in space
struct TokenSkipping
{
	vint											rowSkip = -1;			// row of the first token to skip
	vint											columnSkip = -1;		// column of the first token to skip
	vint											rowUntil = -1;			// row of the first token not to skip
	vint											columnUntil = -1;		// column of the first token not to skip
};

extern void											ReadMappingFile(FilePath pathMapping, Array<TokenSkipping>& mapping);
extern void											WriteMappingFile(FilePath pathMapping, List<TokenSkipping>& mapping);
extern void											PreprocessedFileToCompactCodeAndMapping(Ptr<RegexLexer> lexer, FilePath pathInput, FilePath pathPreprocessed, FilePath pathOutput, FilePath pathMapping);

/***********************************************************************
Indexing
***********************************************************************/

struct IndexToken
{
	vint											rowStart;
	vint											columnStart;
	vint											rowEnd;
	vint											columnEnd;

	static vint										Compare(const IndexToken& a, const IndexToken& b);
	static IndexToken								GetToken(CppName& name);

	DEFINE_COMPLETE_COMPARISON_OPERATOR(IndexToken)
};

enum class IndexReason
{
	Resolved = 0,
	OverloadedResolution = 1,
	NeedValueButType = 2,
	Max = 3,
};

using IndexMap = Group<IndexToken, Symbol*>;
using DeclOrArg = Tuple<Ptr<Declaration>, Symbol*>;

struct IndexResult
{
	// the ParsingArguments used to parse the program
	ParsingArguments								pa;

	// the input code
	WString											input;

	// the output program
	Ptr<Program>									program;

	// unique-symbol-name to symbol mapping created in Compile function
	Dictionary<WString, Symbol*>					ids;

	// indexed token to symbol mapping, continuing tokens are grouped together
	IndexMap										index[(vint)IndexReason::Max];

	// tokens of declaration names, to be highlighted from hyperlinks
	Dictionary<IndexToken, DeclOrArg>				decls;
};

class IProgressReporter : public virtual Interface
{
public:
	virtual void									OnProgress(vint phase, vint totalPhases, vint position, vint length) = 0;
};

class IndexRecorder : public Object, public virtual IIndexRecorder
{
public:
	IndexResult&									result;

	IProgressReporter*								progressReporter;
	vint											currentPhase = 0;
	vint											codeLength = 0;

	IndexRecorder(IndexResult& _result, IProgressReporter* _progressReporter, vint _codeLength);

	void											BeginPhase(Phase phase);

	void											IndexInternal(CppName& name, List<ResolvedItem>& resolvedSymbols, IndexReason reason);
	void											Index(CppName& name, List<ResolvedItem>& resolvedSymbols)override;
	void											IndexOverloadingResolution(CppName& name, List<ResolvedItem>& resolvedSymbols)override;
	void											ExpectValueButType(CppName& name, List<ResolvedItem>& resolvedSymbols)override;
};

/***********************************************************************
Compiling
***********************************************************************/

extern void											Compile(Ptr<RegexLexer> lexer, FilePath pathInput, IndexResult& result, IProgressReporter* progressReporter);
extern void											GenerateUniqueId(IndexResult& result);

/***********************************************************************
Line Indexing
***********************************************************************/

struct HtmlLineRecord
{
	vint											lineCount;		// total lines of this record
	const wchar_t*									rawBegin;		// the first character
	const wchar_t*									rawEnd;			// the one after the last character
	WString											htmlCode;		// generated HTML code
};

struct FileLinesRecord
{
	FilePath										filePath;		// file path of the source file
	WString											htmlFileName;	// file name of the generated HTML file (without ".html")
	Dictionary<vint, HtmlLineRecord>				lines;			// all generated HTML code
	SortedList<Symbol*>								refSymbols;		// symbols of all hyperlink targets in the source file
};

struct GlobalLinesRecord
{
	WString											preprocessed;	// text content of Preprocessed.cpp, ensure that each line ends with a line-break
	Dictionary<FilePath, Ptr<FileLinesRecord>>		fileLines;		// generated HTML code of all touched source files
	Dictionary<DeclOrArg, FilePath>					declToFiles;	// declaration to source file mapping
	SortedList<WString>								htmlFileNames;	// all fileLines[x]->htmlFileName
};

/***********************************************************************
HTML Generating
***********************************************************************/

struct StreamHolder
{
	MemoryStream									memoryStream;
	StreamWriter									streamWriter;

	StreamHolder()
		:streamWriter(memoryStream)
	{
	}
};

extern void											EnumerateDecls(Symbol* symbol, const Func<void(DeclOrArg, bool, vint)>& callback);
extern WString										GetDeclId(DeclOrArg declOrArg);
extern WString										GetSymbolId(Symbol* symbol);
extern const wchar_t*								GetSymbolDivClass(Symbol* symbol);
extern void											WriteHtmlTextSingleLine(const WString& text, StreamWriter& writer);
extern WString										HtmlTextSingleLineToString(const WString& text);
extern void											WriteHtmlAttribute(const WString& text, StreamWriter& writer);

/***********************************************************************
HTML Display Name
***********************************************************************/

extern WString										GetTypeDisplayNameInHtml(Ptr<Type> type, bool renderTypeArguments = true);
extern WString										AppendFunctionParametersInHtml(FunctionType* funcType);
extern WString										AppendTemplateArguments(List<TemplateSpec::Argument>& arguments);
extern WString										AppendGenericArguments(VariadicList<GenericArgument>& arguments);
extern WString										GetUnscopedSymbolDisplayNameInHtml(Symbol* symbol, bool renderTypeArguments);
extern WString										GetSymbolDisplayNameInHtml(Symbol* symbol);

/***********************************************************************
Index Collecting
***********************************************************************/

extern Ptr<GlobalLinesRecord>						Collect(Ptr<RegexLexer> lexer, FilePath pathPreprocessed, FilePath pathInput, FilePath pathMapping, IndexResult& result);

/***********************************************************************
Source Code Page Generating
***********************************************************************/

extern void											GenerateFile(Ptr<GlobalLinesRecord> global, Ptr<FileLinesRecord> flr, IndexResult& result, FilePath pathHtml);

/***********************************************************************
Index Page Generating
***********************************************************************/

// {PathPrefix, Label}
using FileGroupConfig = List<Tuple<WString, WString>>;

extern void											GenerateFileIndex(Ptr<GlobalLinesRecord> global, FilePath pathHtml, FileGroupConfig& fileGroups);
extern void											GenerateSymbolIndex(Ptr<GlobalLinesRecord> global, IndexResult& result, FilePath pathHtml, FileGroupConfig& fileGroups);

#endif