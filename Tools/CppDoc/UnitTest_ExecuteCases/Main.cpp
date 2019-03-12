#include <Parser.h>
#include <Ast_Decl.h>

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

bool NeedToSkip(RegexToken& token)
{
	switch ((CppTokens)token.token)
	{
	case CppTokens::SPACE:
	case CppTokens::COMMENT1:
	case CppTokens::COMMENT2:
	case CppTokens::SHARP:
		return true;
	}
	return false;
}

void CleanUpPreprocessFile(Ptr<RegexLexer> lexer, FilePath pathInput, FilePath pathPreprocessed, FilePath pathOutput, FilePath pathMapping)
{
	WString input = File(pathInput).ReadAllTextByBom();
	File(pathPreprocessed).WriteAllText(input, true, BomEncoder::Utf16);

	List<TokenSkipping> mapping;
	{
		CppTokenReader reader(lexer, input, false);
		auto cursor = reader.GetFirstToken();

		FileStream fileStream(pathOutput.GetFullPath(), FileStream::WriteOnly);
		BomEncoder encoder(BomEncoder::Utf16);
		EncoderStream encoderStream(fileStream, encoder);
		StreamWriter writer(encoderStream);

		while (cursor)
		{
			switch ((CppTokens)cursor->token.token)
			{
			case CppTokens::SPACE:
			case CppTokens::COMMENT1:
			case CppTokens::COMMENT2:
			case CppTokens::SHARP:
				{
					auto oldCursor = cursor;

					TokenSkipping ts;
					ts.rowSkip = cursor->token.rowStart;
					ts.columnSkip = cursor->token.columnStart;

					while (cursor)
					{
						ts.rowUntil = cursor->token.rowStart;
						ts.columnUntil = cursor->token.columnStart;

						if ((CppTokens)cursor->token.token == CppTokens::SHARP)
						{
							vint parenthesisCounter = 0;
							vint lastSharpRowEnd = ts.rowUntil;
							bool lastTokenIsSkipping = false;

							while (cursor)
							{
								ts.rowUntil = cursor->token.rowStart;
								ts.columnUntil = cursor->token.columnStart;
								if (lastTokenIsSkipping && lastSharpRowEnd != ts.rowUntil)
								{
									goto STOP_SHARP;
								}
								lastSharpRowEnd = ts.rowUntil;

								switch ((CppTokens)cursor->token.token)
								{
								case CppTokens::LPARENTHESIS:
									lastTokenIsSkipping = false;
									parenthesisCounter++;
									break;
								case CppTokens::RPARENTHESIS:
									lastTokenIsSkipping = false;
									parenthesisCounter--;
									if (parenthesisCounter == 0)
									{
										cursor = cursor->Next();
										goto STOP_SHARP;
									}
									break;
								case CppTokens::SPACE:
								case CppTokens::COMMENT1:
								case CppTokens::COMMENT2:
									lastTokenIsSkipping = true;
									break;
								default:
									lastTokenIsSkipping = false;
								}
								cursor = cursor->Next();
							}
						STOP_SHARP:;
						}
						else
						{
							switch ((CppTokens)cursor->token.token)
							{
							case CppTokens::SPACE:
							case CppTokens::COMMENT1:
							case CppTokens::COMMENT2:
								break;
							default:
								goto STOP_SKIPPING;
							}
							cursor = cursor->Next();
						}
					}
				STOP_SKIPPING:

					if (ts.rowSkip == ts.rowUntil)
					{
						cursor = oldCursor;
					}
					else
					{
						// prevent from stack overflowing in CppTokenCursor's destructor
						while (oldCursor != cursor)
						{
							oldCursor = oldCursor->Next();
						}
						mapping.Add(ts);
						writer.WriteLine(L"", 0);
					}
				}
			}

			if (cursor)
			{
				writer.WriteString(cursor->token.reading, cursor->token.length);
				cursor = cursor->Next();
			}
		}
	}
	{
		FileStream fileStream(pathMapping.GetFullPath(), FileStream::WriteOnly);
		{
			vint count = mapping.Count();
			fileStream.Write(&count, sizeof(count));
		}
		if (mapping.Count() > 0)
		{
			fileStream.Write(&mapping[0], sizeof(TokenSkipping) * mapping.Count());
		}
	}
}

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

	void IndexInternal(CppName& name, Ptr<Resolving> resolving, IndexReason reason)
	{
		auto key = IndexToken::GetToken(name);
		if (name.tokenCount > 0)
		{
			for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
			{
				auto symbol = resolving->resolvedSymbols[i];
				if (!result.index[(vint)reason].Contains(key, symbol))
				{
					result.index[(vint)reason].Add(key, symbol);
					result.reverseIndex[(vint)reason].Add(symbol, key);
				}
			}
		}
	}

	void Index(CppName& name, Ptr<Resolving> resolving)override
	{
		IndexInternal(name, resolving, IndexReason::Resolved);
	}

	void IndexOverloadingResolution(CppName& name, Ptr<Resolving> resolving)override
	{
		IndexInternal(name, resolving, IndexReason::OverloadedResolution);
	}

	void ExpectValueButType(CppName& name, Ptr<Resolving> resolving)override
	{
		IndexInternal(name, resolving, IndexReason::NeedValueButType);
	}
};

/***********************************************************************
Compiling
***********************************************************************/

void Compile(Ptr<RegexLexer> lexer, FilePath pathFolder, FilePath pathInput, IndexResult& result)
{
	WString input = File(pathInput).ReadAllTextByBom();
	CppTokenReader reader(lexer, input);
	auto cursor = reader.GetFirstToken();

	result.pa = { new Symbol, ITsysAlloc::Create(), new IndexRecorder(result) };
	auto program = ParseProgram(result.pa, cursor);
	EvaluateProgram(result.pa, program);

	result.pa.root->GenerateUniqueId(result.ids, L"");
	for (vint i = 0; i < result.ids.Count(); i++)
	{
		auto symbol = result.ids.Values()[i];
		if (symbol->definition)
		{
			auto& name = symbol->definition->name;
			if (name.tokenCount > 0)
			{
				result.decls.Add(IndexToken::GetToken(name), symbol->definition);
			}
		}
		for (vint i = 0; i < symbol->declarations.Count(); i++)
		{
			auto decl = symbol->declarations[i];
			auto& name = decl->name;
			if (name.tokenCount > 0)
			{
				result.decls.Add(IndexToken::GetToken(name), decl);
			}
		}
	}
}

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

void AdjustSkippingIndex(Ptr<CppTokenCursor>& cursor, Array<TokenSkipping>& skipping, IndexTracking& index, AdjustSkippingResult& asr)
{
	auto& token = cursor->token;

	while (true)
	{
		if (index.index >= skipping.Count())
		{
			index.inRange = false;
			return;
		}

		auto& current = skipping[index.index];
		if (token.rowStart > current.rowUntil || (token.rowStart == current.rowUntil && token.columnStart >= current.columnUntil))
		{
			asr.rowSkipped += current.rowUntil - current.rowSkip - 1;
			index.index++;
		}
		else
		{
			break;
		}
	}

	auto& current = skipping[index.index];
	if (token.rowStart < current.rowSkip || (token.rowStart == current.rowSkip && token.columnStart < current.columnSkip))
	{
		index.inRange = false;
		return;
	}

	index.inRange = true;
	asr.rowUntil = current.rowUntil;
	asr.columnUntil = current.columnUntil;
}

void AdjustRefIndex(Ptr<CppTokenCursor>& cursor, const SortedList<IndexToken>& keys, IndexTracking& index, const AdjustSkippingResult& asr)
{
	vint row = cursor->token.rowStart;
	vint column = cursor->token.columnStart;
	if (row == asr.rowUntil)
	{
		column -= asr.columnUntil;
	}
	row -= asr.rowSkipped;

	while (true)
	{
		if (index.index >= keys.Count())
		{
			index.inRange = false;
			return;
		}

		auto& current = keys[index.index];
		if (row > current.rowEnd || (row == current.rowEnd && column > current.columnEnd))
		{
			index.index++;
		}
		else
		{
			break;
		}
	}

	auto& current = keys[index.index];
	if (row < current.rowStart || (row == current.rowStart && column < current.columnStart))
	{
		index.inRange = false;
		return;
	}

	index.inRange = true;
}

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
	WString											displayName;
	Dictionary<vint, HtmlLineRecord>				lines;
	SortedList<Symbol*>								refSymbols;
};

struct GlobalLinesRecord
{
	WString											preprocessed;
	Dictionary<WString, Ptr<FileLinesRecord>>		fileLines;
	Dictionary<Ptr<Declaration>, WString>			declToFiles;
	SortedList<WString>								displayNames;
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

WString Submit(Ptr<StreamHolder>& holder)
{
	if (!holder) return WString::Empty;
	holder->memoryStream.SeekFromBegin(0);
	auto result = StreamReader(holder->memoryStream).ReadToEnd();
	holder = nullptr;
	return result;
}

StreamWriter& Use(Ptr<StreamHolder>& holder)
{
	if (!holder)
	{
		holder = new StreamHolder;
	}
	return holder->streamWriter;
}

template<typename T>
void GenerateHtmlToken(
	Ptr<CppTokenCursor>& cursor,
	IndexResult& result,
	Symbol* symbolForToken,
	IndexTracking (&indexResolve)[(vint)IndexReason::Max],
	const wchar_t*& rawBegin,
	const wchar_t*& rawEnd,
	Ptr<StreamHolder>& html,
	vint& lineCounter,
	const T& callback
)
{
	const wchar_t* divClass = nullptr;

	switch ((CppTokens)cursor->token.token)
	{
	case CppTokens::DOCUMENT:
	case CppTokens::COMMENT1:
	case CppTokens::COMMENT2:
		divClass = L"cpp_comment ";
		break;
	case CppTokens::STRING:
	case CppTokens::CHAR:
		divClass = L"cpp_string ";
		break;
	case CppTokens::INT:
	case CppTokens::HEX:
	case CppTokens::BIN:
	case CppTokens::FLOAT:
		divClass = L"cpp_number ";
		break;
#define CASE_KEYWORD(NAME, REGEX) case CppTokens::NAME:
		CPP_KEYWORD_TOKENS(CASE_KEYWORD)
#undef CASE_KEYWORD
			divClass = L"cpp_keyword ";
		break;
	default:
		if (symbolForToken)
		{
			switch (symbolForToken->kind)
			{
			case symbol_component::SymbolKind::Enum:
			case symbol_component::SymbolKind::Class:
			case symbol_component::SymbolKind::Struct:
			case symbol_component::SymbolKind::Union:
			case symbol_component::SymbolKind::TypeAlias:
				divClass = L"cpp_type";
				break;
			case symbol_component::SymbolKind::EnumItem:
				divClass = L"cpp_enum";
				break;
			case symbol_component::SymbolKind::Variable:
				if (symbolForToken->parent)
				{
					if (symbolForToken->parent->definition.Cast<FunctionDeclaration>())
					{
						divClass = L"cpp_argument";
					}
					else if (symbolForToken->parent->definition.Cast<ClassDeclaration>())
					{
						divClass = L"cpp_field";
					}
				}
				break;
			case symbol_component::SymbolKind::Function:
				divClass = L"cpp_function";
				break;
			}
		}
	}

	if (divClass)
	{
		Use(html).WriteString(L"<div class=\"");
		if (divClass)
		{
			Use(html).WriteString(L"token ");
			Use(html).WriteString(divClass);
		}
		Use(html).WriteString(L"\">");
	}

	auto reading = cursor->token.reading;
	auto length = cursor->token.length;
	bool canLineBreak = !symbolForToken && !divClass && (CppTokens)cursor->token.token == CppTokens::SPACE;
	for (vint i = 0; i < length; i++)
	{
		switch (reading[i])
		{
		case L'\r':
			if (canLineBreak)
			{
				rawEnd = &reading[i];
			}
			break;
		case L'\n':
			if (canLineBreak)
			{
				// the last token is always a space, so it is not necessary to submit a line after all cursor is read
				if (rawEnd != &reading[i - 1])
				{
					rawEnd = &reading[i];
				}
				lineCounter++;
				callback({ lineCounter, rawBegin, rawEnd, Submit(html) });

				lineCounter = 0;
				rawBegin = &reading[i + 1];
				rawEnd = rawBegin;
			}
			else
			{
				Use(html).WriteLine(L"");
			}
			break;
		case L'<':
			Use(html).WriteString(L"&lt;");
			break;
		case L'>':
			Use(html).WriteString(L"&gt;");
			break;
		case L'&':
			Use(html).WriteString(L"&amp;");
			break;
		case L'\'':
			Use(html).WriteString(L"&apos;");
			break;
		case L'\"':
			Use(html).WriteString(L"&quot;");
			break;
		default:
			Use(html).WriteChar(reading[i]);
		}
	}

	if (divClass)
	{
		Use(html).WriteString(L"</div>");
	}
}

template<typename T>
void GenerateHtmlLine(Ptr<CppTokenCursor>& cursor, Ptr<GlobalLinesRecord> global, const WString& currentFilePath, Array<TokenSkipping>& skipping, IndexResult& result, const T& callback)
{
	if (!cursor) return;
	const wchar_t* rawBegin = cursor->token.reading;
	const wchar_t * rawEnd = rawBegin;
	Ptr<StreamHolder> html;
	vint lineCounter = 0;

	AdjustSkippingResult asr;
	IndexTracking indexSkipping, indexDecl, indexResolve[(vint)IndexReason::Max];
	bool lastTokenIsDef = false;
	bool lastTokenIsRef = false;
	while (cursor)
	{
		// calculate the surrounding context of the current token
		AdjustSkippingIndex(cursor, skipping, indexSkipping, asr);
		if (!indexSkipping.inRange)
		{
			AdjustRefIndex(cursor, result.decls.Keys(), indexDecl, asr);
			for (vint i = 0; i < (vint)IndexReason::Max; i++)
			{
				AdjustRefIndex(cursor, result.index[i].Keys(), indexResolve[i], asr);
			}
		}

		// a link is not possible to be the last token of a valid C++ file, so this should just work
		bool isDefToken = indexDecl.inRange;
		bool isRefToken = indexResolve[(vint)IndexReason::Resolved].inRange || indexResolve[(vint)IndexReason::OverloadedResolution].inRange;

		if (isDefToken && !lastTokenIsDef)
		{
			auto decl = result.decls.Values()[indexDecl.index];
			global->declToFiles.Add(decl, currentFilePath);

			Use(html).WriteString(L"<div class=\"def\" id=\"");
			if (decl == decl->symbol->definition)
			{
				Use(html).WriteString(L"Decl$");
			}
			else
			{
				Use(html).WriteString(L"Forward[" + itow(decl->symbol->declarations.IndexOf(decl.Obj())) + L"]$");
			}
			Use(html).WriteString(decl->symbol->uniqueId);
			Use(html).WriteString(L"\">");

			if ((decl->symbol->definition ? 1 : 0) + decl->symbol->declarations.Count() > 1)
			{
				Use(html).WriteString(L"<div class=\"ref\" onclick=\"jumpToSymbol([], [\'");
				Use(html).WriteString(decl->symbol->uniqueId);
				Use(html).WriteString(L"\'])\">");
			}
			else
			{
				Use(html).WriteString(L"<div>");
			}
		}
		else if(!isDefToken && lastTokenIsDef)
		{
			Use(html).WriteString(L"</div></div>");
		}

		if (isRefToken && !lastTokenIsRef)
		{
			if (isDefToken)
			{
				throw Exception(L"It is not possible for a token to be a definition and a reference at the same time.");
			}
			auto flr = global->fileLines[currentFilePath];
			for (vint i = (vint)IndexReason::OverloadedResolution; i >= (vint)IndexReason::Resolved; i--)
			{
				if (indexResolve[i].inRange)
				{
					auto& symbols = result.index[i].GetByIndex(indexResolve[i].index);
					for (vint j = 0; j < symbols.Count(); j++)
					{
						auto symbol = symbols[j];
						if (!flr->refSymbols.Contains(symbol))
						{
							flr->refSymbols.Add(symbol);
						}
					}
				}
			}

			Use(html).WriteString(L"<div class=\"ref\" onclick=\"jumpToSymbol([");
			for (vint i = (vint)IndexReason::OverloadedResolution; i >= (vint)IndexReason::Resolved; i--)
			{
				if (i != 0) Use(html).WriteString(L"], [");
				if (indexResolve[i].inRange)
				{
					auto& symbols = result.index[i].GetByIndex(indexResolve[i].index);
					for (vint j = 0; j < symbols.Count(); j++)
					{
						if (j != 0) Use(html).WriteString(L", ");
						Use(html).WriteString(L"\'");
						Use(html).WriteString(symbols[j]->uniqueId);
						Use(html).WriteString(L"\'");
					}
					break;
				}
			}
			Use(html).WriteString(L"])\">");
		}
		else if (!isRefToken && lastTokenIsRef)
		{
			Use(html).WriteString(L"</div>");
		}
		lastTokenIsDef = isDefToken;
		lastTokenIsRef = isRefToken;

		// write a token
		Symbol* symbolForToken = nullptr;
		if (isDefToken)
		{
			symbolForToken = result.decls.Values()[indexDecl.index]->symbol;
		}
		else if (isRefToken)
		{
			for (vint i = (vint)IndexReason::OverloadedResolution; i >= (vint)IndexReason::Resolved; i--)
			{
				if (indexResolve[i].inRange)
				{
					symbolForToken = result.index[i].GetByIndex(indexResolve[i].index)[0];
					break;
				}
			}
		}
		GenerateHtmlToken(cursor, result, symbolForToken, indexResolve, rawBegin, rawEnd, html, lineCounter, callback);

		cursor = cursor->Next();
		if (cursor && (CppTokens)cursor->token.token == CppTokens::SHARP)
		{
			// let the outside decide whether this # need to be generate HTML code or not
			break;
		}
	}
}

/***********************************************************************
Index Collecting
***********************************************************************/

Ptr<GlobalLinesRecord> Collect(Ptr<RegexLexer> lexer, const WString& title, FilePath pathPreprocessed, FilePath pathInput, FilePath pathMapping, IndexResult& result)
{
	auto global = MakePtr<GlobalLinesRecord>();
	Array<TokenSkipping> skipping;
	{
		FileStream fileStream(pathMapping.GetFullPath(), FileStream::ReadOnly);
		vint count;
		fileStream.Read(&count, sizeof(count));
		skipping.Resize(count);

		if (count > 0)
		{
			fileStream.Read(&skipping[0], sizeof(TokenSkipping)*count);
		}
	}

	global->preprocessed = File(pathPreprocessed).ReadAllTextByBom();
	if (global->preprocessed.Right(1) != L"\n")
	{
		global->preprocessed += L"\r\n";
	}

	CppTokenReader reader(lexer, global->preprocessed, false);
	auto cursor = reader.GetFirstToken();

	vint currentLineNumber = 0;
	WString currentFilePath;
	bool rightAfterSharpLine = false;
	while (cursor)
	{
		{
			auto oldCursor = cursor;
			{
				if (!TestToken(cursor, CppTokens::SHARP)) goto GIVE_UP;
				if (!TestToken(cursor, L"line")) goto GIVE_UP;
				if (!TestToken(cursor, CppTokens::SPACE)) goto GIVE_UP;
				if (!TestToken(cursor, CppTokens::INT, false)) goto GIVE_UP;
				vint lineNumber = wtoi(WString(cursor->token.reading, cursor->token.length));
				SkipToken(cursor);
				if (!TestToken(cursor, CppTokens::SPACE)) goto GIVE_UP;
				if (!TestToken(cursor, CppTokens::STRING, false)) goto GIVE_UP;
				Array<wchar_t> buffer(cursor->token.length - 2);
				auto reading = cursor->token.reading + 1;
				auto writing = &buffer[0];
				while (*reading != L'\"')
				{
					if (*reading == L'\\')
					{
						reading++;
					}
					*writing++ = *reading++;
				}

				currentLineNumber = lineNumber - 1;
				currentFilePath = wupper(WString(&buffer[0], (vint)(writing - &buffer[0])));
				rightAfterSharpLine = true;

				{
					vint fileIndex = global->fileLines.Keys().IndexOf(currentFilePath);
					if (fileIndex == -1)
					{
						auto flr = MakePtr<FileLinesRecord>();
						flr->filePath = currentFilePath;

						WString displayName = flr->filePath.GetName();
						vint counter = 1;
						while (true)
						{
							flr->displayName = displayName + (counter == 1 ? WString::Empty : itow(counter));
							if (!global->displayNames.Contains(flr->displayName))
							{
								global->displayNames.Add(flr->displayName);
								break;
							}
							counter++;

						}
						global->fileLines.Add(currentFilePath, flr);
					}
				}
				SkipToken(cursor);
				continue;
			}
		GIVE_UP:
			cursor = oldCursor;
		}
		GenerateHtmlLine(cursor, global, currentFilePath, skipping, result, [&](HtmlLineRecord hlr)
		{
			if (rightAfterSharpLine)
			{
				if (hlr.htmlCode.Length() != 0)
				{
					throw Exception(L"An empty line should have been submitted right after #line.");
				}
				rightAfterSharpLine = false;
			}
			else
			{
				auto flr = global->fileLines[currentFilePath];
				flr->lines.Add(currentLineNumber, hlr);
				currentLineNumber += hlr.lineCount;
			}
		});
	}

	return global;
}

/***********************************************************************
File Generating
***********************************************************************/

void GenerateFile(Ptr<GlobalLinesRecord> global, Ptr<FileLinesRecord> flr, IndexResult& result, FilePath pathHtml)
{
	FileStream fileStream(pathHtml.GetFullPath(), FileStream::WriteOnly);
	Utf8Encoder encoder;
	EncoderStream encoderStream(fileStream, encoder);
	StreamWriter writer(encoderStream);

	writer.WriteLine(L"<!DOCTYPE html>");
	writer.WriteLine(L"<html>");
	writer.WriteLine(L"<head>");
	writer.WriteLine(L"    <title>" + flr->filePath.GetName() + L"</title>");
	writer.WriteLine(L"    <link rel=\"stylesheet\" href=\"../Cpp.css\" />");
	writer.WriteLine(L"    <link rel=\"shortcut icon\" href=\"../favicon.ico\" />");
	writer.WriteLine(L"    <script type=\"text/javascript\" src=\"../Cpp.js\" ></script>");
	writer.WriteLine(L"</head>");
	writer.WriteLine(L"<body>");
	writer.WriteString(L"<div class=\"codebox\"><div class=\"cpp_default\">");

	{
		List<WString> originalLines;
		File(flr->filePath).ReadAllLinesByBom(originalLines);

		vint originalIndex = 0;
		vint flrIndex = 0;
		while (originalIndex < originalLines.Count())
		{
			vint disableEnd = -1;
			vint nextProcessingLine = -1;
			WString embedHtmlInDisabled;

			if (flrIndex == flr->lines.Count())
			{
				disableEnd = originalLines.Count();
				nextProcessingLine = disableEnd;
			}
			else
			{
				vint nextAvailable = flr->lines.Keys()[flrIndex];
				if (originalIndex < nextAvailable)
				{
					disableEnd = nextAvailable;
					nextProcessingLine = disableEnd;
				}
				else if (originalIndex == nextAvailable)
				{
					auto& currentHtmlLines = flr->lines.Values()[flrIndex++];
					if (flrIndex == flr->lines.Count())
					{
						nextProcessingLine = originalLines.Count();
					}
					else
					{
						nextProcessingLine = flr->lines.Keys()[flrIndex];
					}

					bool macroExpanded = (nextProcessingLine - originalIndex) != currentHtmlLines.lineCount;
					if (!macroExpanded)
					{
						StringReader reader(WString(currentHtmlLines.rawBegin, (vint)(currentHtmlLines.rawEnd - currentHtmlLines.rawBegin)));
						for (vint i = originalIndex; i < nextProcessingLine; i++)
						{
							if (originalLines[i] != reader.ReadLine())
							{
								macroExpanded = true;
								break;
							}
						}
					}

					if (!macroExpanded)
					{
						writer.WriteLine(currentHtmlLines.htmlCode);
					}
					else
					{
						bool allSpaces = true;
						for (auto reading = currentHtmlLines.rawBegin; allSpaces && reading < currentHtmlLines.rawEnd; reading++)
						{
							switch (*reading++)
							{
							case L' ':
							case L'\t':
							case L'\r':
							case L'\n':
							case L'\v':
							case L'\f':
								break;
							default:
								allSpaces = false;
							}
						}

						if (!allSpaces)
						{
							embedHtmlInDisabled = currentHtmlLines.htmlCode;
						}
						disableEnd = nextProcessingLine;
					}
				}
				else
				{
					throw Exception(L"Too many lines are processed.");
				}
			}

			if (disableEnd != -1)
			{
				writer.WriteString(L"<div class=\"disabled\"/>");
				while (originalIndex < disableEnd)
				{
					writer.WriteLine(L"");
					writer.WriteString(originalLines[originalIndex]);
					originalIndex++;
				}
				writer.WriteLine(L"</div>");
				if (embedHtmlInDisabled.Length() != 0)
				{
					writer.WriteLine(embedHtmlInDisabled);
				}
			}
			originalIndex = nextProcessingLine;
		}
	}

	writer.WriteLine(L"</div></div>");

	writer.WriteLine(L"<script type=\"text/javascript\">");

	writer.WriteLine(L"referencedSymbols = {");
	for (vint i = 0; i < flr->refSymbols.Count(); i++)
	{
		auto symbol = flr->refSymbols[i];
		writer.WriteString(L"    \'");
		writer.WriteString(symbol->uniqueId);
		writer.WriteString(L"\': { \'name\': \'");
		writer.WriteString(symbol->name);
		writer.WriteString(L"\', \'definition\': ");
		writer.WriteString(symbol->definition ? L"true" : L"false");
		writer.WriteString(L", \'declarations\': ");
		writer.WriteString(itow(symbol->declarations.Count()));
		if (i == flr->refSymbols.Count() - 1)
		{
			writer.WriteLine(L"}");
		}
		else
		{
			writer.WriteLine(L"},");
		}
	}
	writer.WriteLine(L"};");

	writer.WriteLine(L"symbolToFiles = {");
	bool firstFileMapping = false;
	auto writeFileMapping = [&](const WString& prefix, Ptr<Declaration> decl)
	{
		vint index = global->declToFiles.Keys().IndexOf(decl.Obj());
		if (index == -1) return;

		if (!firstFileMapping)
		{
			firstFileMapping = true;
		}
		else
		{
			writer.WriteLine(L",");
		}

		writer.WriteString(L"    \'");
		writer.WriteString(prefix);
		writer.WriteString(decl->symbol->uniqueId);
		writer.WriteString(L"\': \'");

		auto filePath = global->declToFiles.Values()[index];
		auto flrTarget = global->fileLines[filePath];
		if (flrTarget != flr)
		{
			writer.WriteString(global->fileLines[filePath]->displayName);
		}
		writer.WriteString(L"\'");
	};
	for (vint i = 0; i < flr->refSymbols.Count(); i++)
	{
		auto symbol = flr->refSymbols[i];
		if (symbol->definition)
		{
			auto decl = symbol->definition;
			writeFileMapping(L"Decl$", decl);
		}
		for (vint j = 0; j < symbol->declarations.Count(); j++)
		{
			auto decl = symbol->declarations[j];
			writeFileMapping(L"Forward[" + itow(j) + L"]$", decl);
		}
	}
	writer.WriteLine(L"");
	writer.WriteLine(L"};");
	writer.WriteLine(L"turnOnSymbol();");

	writer.WriteLine(L"</script>");
	writer.WriteLine(L"</body>");
	writer.WriteLine(L"</html>");
}

/***********************************************************************
Main
***********************************************************************/

int main()
{
	Folder folderCase(L"../UnitTest_Cases");
	List<File> files;
	folderCase.GetFiles(files);

	Console::WriteLine(L"Cleaning ...");
	FOREACH(File, file, files)
	{
		if (wupper(file.GetFilePath().GetFullPath().Right(2)) == L".I")
		{
			Folder folderOutput(file.GetFilePath().GetFullPath() + L".Output");
			if (folderOutput.Exists())
			{
				folderOutput.Delete(true);
			}
		}
	}
	auto lexer = CreateCppLexer();

	FOREACH(File, file, files)
	{
		if (wupper(file.GetFilePath().GetFullPath().Right(2)) == L".I")
		{
			Folder folderOutput(file.GetFilePath().GetFullPath() + L".Output");
			auto pathPreprocessed = folderOutput.GetFilePath() / L"Preprocessed.cpp";
			auto pathInput = folderOutput.GetFilePath() / L"Input.cpp";
			auto pathMapping = folderOutput.GetFilePath() / L"Mapping.bin";

			if (!folderOutput.Exists())
			{
				folderOutput.Create(false);
			}

			Console::WriteLine(L"Preprocessing " + file.GetFilePath().GetName() + L" ...");
			CleanUpPreprocessFile(
				lexer,
				file.GetFilePath(),
				pathPreprocessed,
				pathInput,
				pathMapping
			);

			Console::WriteLine(L"Compiling " + file.GetFilePath().GetName() + L" ...");
			IndexResult indexResult;
			Compile(
				lexer,
				file.GetFilePath(),
				pathInput,
				indexResult
			);

			Console::WriteLine(L"Generating HTML for " + file.GetFilePath().GetName() + L" ...");
			auto global = Collect(
				lexer,
				file.GetFilePath().GetName(),
				pathPreprocessed,
				pathInput,
				pathMapping,
				indexResult
			);

			for (vint i = 0; i < global->fileLines.Keys().Count(); i++)
			{
				auto flr = global->fileLines.Values()[i];
				GenerateFile(global, flr, indexResult, folderOutput.GetFilePath() / (flr->displayName + L".html"));
			}
		}
	}

	return 0;
}