#ifndef VCZH_DOCUMENT_CPPDOC_RENDER
#define VCZH_DOCUMENT_CPPDOC_RENDER

#include <VlppOS.h>
#include <Ast_Resolving.h>

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

struct IndexResult
{
	ParsingArguments								pa;
	Dictionary<WString, Symbol*>					ids;
	IndexMap										index[(vint)IndexReason::Max];
	Dictionary<IndexToken, Ptr<Declaration>>		decls;
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

/***********************************************************************
Token Indexing
***********************************************************************/

struct AdjustSkippingResult
{
	vint											rowSkipped = 0;
	vint											rowUntil = 0;
	vint											columnUntil = 0;
};

struct IndexTracking
{
	vint											index = 0;
	bool											inRange = false;
};

/***********************************************************************
Line Indexing
***********************************************************************/

struct HtmlLineRecord
{
	vint											lineCount;
	const wchar_t*									rawBegin;
	const wchar_t*									rawEnd;
	WString											htmlCode;
};

struct FileLinesRecord
{
	FilePath										filePath;
	WString											htmlFileName;
	Dictionary<vint, HtmlLineRecord>				lines;
	SortedList<Symbol*>								refSymbols;
};

struct GlobalLinesRecord
{
	WString											preprocessed;
	Dictionary<FilePath, Ptr<FileLinesRecord>>		fileLines;
	Dictionary<Ptr<Declaration>, FilePath>			declToFiles;
	SortedList<WString>								htmlFileNames;
};

struct TokenTracker
{
	AdjustSkippingResult							asr;
	IndexTracking									indexSkipping;
	IndexTracking									indexDecl;
	IndexTracking									indexResolve[(vint)IndexReason::Max];
	bool											lastTokenIsDef = false;
	bool											lastTokenIsRef = false;
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

extern void											EnumerateDecls(Symbol* symbol, const Func<void(Ptr<Declaration>, bool, vint)>& callback);
extern WString										GetDeclId(Ptr<Declaration> decl);
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