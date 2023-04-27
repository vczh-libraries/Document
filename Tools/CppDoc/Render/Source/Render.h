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

	static IndexToken								GetToken(CppName& name);

	std::strong_ordering operator<=>(const IndexToken&) const = default;
};

enum class IndexReason
{
	Resolved = 0,
	OverloadedResolution = 1,
	Max = 3,
};

using IndexMap = Group<IndexToken, Symbol*>;

struct DeclOrArg
{
	Ptr<Declaration>								decl;
	Symbol*											symbol = nullptr;

	DeclOrArg() = default;
	DeclOrArg(const Ptr<Declaration>& _decl, Symbol* _symbol)
		:decl(_decl)
		, symbol(_symbol)
	{
	}

	std::strong_ordering operator<=>(const DeclOrArg&) const = default;
};

struct IndexedDeclOrArg : DeclOrArg
{
	IndexToken										token;

	IndexedDeclOrArg() = default;
	IndexedDeclOrArg(const IndexToken& _token, const Ptr<Declaration>& _decl, Symbol* _symbol)
		:DeclOrArg(_decl, _symbol)
		, token(_token)
	{
	}
};

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
	List<IndexedDeclOrArg>							decls;
};

class IProgressReporter : public virtual Interface
{
public:
	enum class ExtraPhases
	{
		UniqueId = (vint)IIndexRecorder::Phase::Finished,
		HTML,
		SymbolIndex,
		ReferenceIndex,
	};

	virtual void									OnProgress(vint phase, vint position, vint length) = 0;
};

class IndexRecorder : public Object, public virtual IIndexRecorder
{
public:
	IndexResult&									result;

	IProgressReporter*								progressReporter;
	vint											currentPhase = 0;
	vint											codeLength = 0;
	vint											stack = 0;

	IndexRecorder(IndexResult& _result, IProgressReporter* _progressReporter, vint _codeLength);

	void											BeginPhase(Phase phase)override;
	void											BeginDelayParse(FunctionDeclaration* decl)override;
	void											EndDelayParse(FunctionDeclaration* decl)override;
	void											BeginEvaluate(Declaration* decl)override;
	void											EndEvaluate(Declaration* decl)override;

	void											IndexInternal(CppName& name, List<ResolvedItem>& resolvedSymbols, IndexReason reason);
	void											Index(CppName& name, List<ResolvedItem>& resolvedSymbols)override;
	void											IndexOverloadingResolution(CppName& name, List<ResolvedItem>& resolvedSymbols)override;
};

/***********************************************************************
Compiling
***********************************************************************/

extern void											Compile(Ptr<RegexLexer> lexer, FilePath pathInput, IndexResult& result, IProgressReporter* progressReporter);
extern void											GenerateUniqueId(IndexResult& result, IProgressReporter* progressReporter);

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

struct DocumentRecord
{
	Ptr<Declaration>								decl;			// the declaration that the document applies to
	List<RegexToken>								comments;		// consecutive tokens of CppTokens::DOCUMENT
};

struct GlobalLinesRecord
{
	WString											preprocessed;	// text content of Preprocessed.cpp, ensure that each line ends with a line-break
	Dictionary<FilePath, Ptr<FileLinesRecord>>		fileLines;		// generated HTML code of all touched source files
	Dictionary<DeclOrArg, FilePath>					declToFiles;	// declaration to source file mapping
	Dictionary<Symbol*, Ptr<DocumentRecord>>		declComments;	// declaration to document comments
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
extern WString										GetUnscopedSymbolDisplayNameInHtml(Symbol* symbol, bool renderTypeArguments);
extern WString										GetSymbolDisplayNameInHtml(Symbol* symbol);

extern WString										AppendGenericArgumentsInSignature(VariadicList<GenericArgument>& arguments);
extern WString										GetTypeDisplayNameInSignature(Ptr<Type> type);
extern WString										GetTypeDisplayNameInSignature(Ptr<Type> type, const WString& signature, bool needParenthesesForFuncArray, FunctionType* topLevelFunctionType = nullptr);
extern WString										GetSymbolDisplayNameInSignature(Symbol* symbol, SortedList<Symbol*>& seeAlsos, SortedList<Symbol*>& baseTypes);

/***********************************************************************
Index Collecting
***********************************************************************/

extern Ptr<GlobalLinesRecord>						Collect(Ptr<RegexLexer> lexer, FilePath pathPreprocessed, FilePath pathInput, FilePath pathMapping, IndexResult& result, IProgressReporter* progressReporter);

/***********************************************************************
Source Code Page Generating
***********************************************************************/

extern void											GenerateFile(Ptr<GlobalLinesRecord> global, Ptr<FileLinesRecord> flr, IndexResult& result, FilePath pathHtml);

/***********************************************************************
Index Page Generating
***********************************************************************/

enum class SymbolGroupKind
{
	Root,
	Group,
	Text,
	Symbol,
	SymbolAndText,
};

struct SymbolGroup
{
	SymbolGroupKind					kind = SymbolGroupKind::Symbol;		// category of this symbol
	WString							uniqueId;							// unique id for file name generation
	WString							name;								// for SymbolGroupKind::Group/Text/SymbolAndText
	Symbol*							symbol = nullptr;					// for SymbolGroupKind::Symbol/SymbolAndText
	bool							braces = false;						// switch of generate braces in SymbolIndex.html
	bool							hasDocument = false;				// true if assignedDocument is true or hasDocument in any children is true
	bool							assignedDocument = false;			// true if a document is assigned to this node
	List<Ptr<SymbolGroup>>			children;							// child symbols
};

// {PathPrefix, Label}
using FileGroupConfig = List<Tuple<WString, WString>>;

extern void											GenerateFileIndex(Ptr<GlobalLinesRecord> global, FilePath pathHtml, FileGroupConfig& fileGroups);
extern Ptr<SymbolGroup>								GenerateSymbolIndex(Ptr<GlobalLinesRecord> global, IndexResult& result, FilePath pathHtml, FilePath pathFragment, FileGroupConfig& fileGroups, IProgressReporter* progressReporter);
extern void											GenerateReferenceIndex(Ptr<GlobalLinesRecord> global, IndexResult& result, Ptr<SymbolGroup> rootGroup, FilePath pathXml, FilePath pathReference, FileGroupConfig& fileGroups, SortedList<WString>& predefinedGroups, IProgressReporter* progressReporter);


/***********************************************************************
Entry
***********************************************************************/

void IndexCppCode(
	FileGroupConfig&		fileGroups,
	File					preprocessedFile,
	Ptr<RegexLexer>			lexer,

	FilePath				pathPreprocessed,
	FilePath				pathInput,
	FilePath				pathMapping,

	Folder					folderOutput,
	Folder					folderSource,
	Folder					folderFragment,
	Folder					folderReference
);

#endif