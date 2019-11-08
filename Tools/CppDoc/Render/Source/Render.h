#ifndef VCZH_DOCUMENT_CPPDOC_RENDER
#define VCZH_DOCUMENT_CPPDOC_RENDER

#include <VlppOS.h>
#include <Parser.h>
#include <Ast_Decl.h>
#include <Ast_Type.h>

using namespace vl::stream;
using namespace vl::filesystem;
using namespace vl::console;

/***********************************************************************
Preprocessing
***********************************************************************/

// skip these tokens to in space
struct TokenSkipping
{
	vint											rowSkip = -1;
	vint											columnSkip = -1;
	vint											rowUntil = -1;
	vint											columnUntil = -1;
};

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

	static vint Compare(const IndexToken& a, const IndexToken& b)
	{
		vint result;
		if ((result = a.rowStart - b.rowStart) != 0) return result;
		if ((result = a.columnStart - b.columnStart) != 0) return result;
		if ((result = a.rowEnd - b.rowEnd) != 0) return result;
		if ((result = a.columnEnd - b.columnEnd) != 0) return result;
		return 0;
	}

	bool operator <  (const IndexToken& k)const { return Compare(*this, k) < 0; }
	bool operator <= (const IndexToken& k)const { return Compare(*this, k) <= 0; }
	bool operator >  (const IndexToken& k)const { return Compare(*this, k) > 0; }
	bool operator >= (const IndexToken& k)const { return Compare(*this, k) >= 0; }
	bool operator == (const IndexToken& k)const { return Compare(*this, k) == 0; }
	bool operator != (const IndexToken& k)const { return Compare(*this, k) != 0; }

	static IndexToken GetToken(CppName& name)
	{
		return {
			name.nameTokens[0].rowStart,
			name.nameTokens[0].columnStart,
			name.nameTokens[name.tokenCount - 1].rowEnd,
			name.nameTokens[name.tokenCount - 1].columnEnd,
		};
	}
};

enum class IndexReason
{
	Resolved = 0,
	OverloadedResolution = 1,
	NeedValueButType = 2,
	Max = 3,
};

using IndexMap = Group<IndexToken, Symbol*>;
using ReverseIndexMap = Group<Symbol*, IndexToken>;

struct IndexResult
{
	ParsingArguments								pa;
	Dictionary<WString, Symbol*>					ids;
	IndexMap										index[(vint)IndexReason::Max];
	ReverseIndexMap									reverseIndex[(vint)IndexReason::Max];
	Dictionary<IndexToken, Ptr<Declaration>>		decls;
};

class IndexRecorder : public Object, public virtual IIndexRecorder
{
public:
	IndexResult&				result;

	IndexRecorder(IndexResult& _result)
		:result(_result)
	{
	}

	void IndexInternal(CppName& name, List<Symbol*>& resolvedSymbols, IndexReason reason)
	{
		auto key = IndexToken::GetToken(name);
		if (name.tokenCount > 0)
		{
			for (vint i = 0; i < resolvedSymbols.Count(); i++)
			{
				auto symbol = resolvedSymbols[i];
				if (!result.index[(vint)reason].Contains(key, symbol))
				{
					result.index[(vint)reason].Add(key, symbol);
					result.reverseIndex[(vint)reason].Add(symbol, key);
				}
			}
		}
	}

	void Index(CppName& name, List<Symbol*>& resolvedSymbols)override
	{
		IndexInternal(name, resolvedSymbols, IndexReason::Resolved);
	}

	void IndexOverloadingResolution(CppName& name, List<Symbol*>& resolvedSymbols)override
	{
		IndexInternal(name, resolvedSymbols, IndexReason::OverloadedResolution);
	}

	void ExpectValueButType(CppName& name, List<Symbol*>& resolvedSymbols)override
	{
		IndexInternal(name, resolvedSymbols, IndexReason::NeedValueButType);
	}
};

/***********************************************************************
Compiling
***********************************************************************/

extern void											Compile(Ptr<RegexLexer> lexer, FilePath pathFolder, FilePath pathInput, IndexResult& result);

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

/***********************************************************************
Index Collecting
***********************************************************************/

extern Ptr<GlobalLinesRecord>						Collect(Ptr<RegexLexer> lexer, const WString& title, FilePath pathPreprocessed, FilePath pathInput, FilePath pathMapping, IndexResult& result);

/***********************************************************************
Source Code Page Generating
***********************************************************************/

extern void											GenerateFile(Ptr<GlobalLinesRecord> global, Ptr<FileLinesRecord> flr, IndexResult& result, FilePath pathHtml);

/***********************************************************************
Index Page Generating
***********************************************************************/

using FileGroupConfig = List<Tuple<WString, WString>>;

extern void											GenerateFileIndex(Ptr<GlobalLinesRecord> global, FilePath pathHtml, FileGroupConfig& fileGroups);
extern void											GenerateSymbolIndex(Ptr<GlobalLinesRecord> global, IndexResult& result, FilePath pathHtml, FileGroupConfig& fileGroups);

#endif