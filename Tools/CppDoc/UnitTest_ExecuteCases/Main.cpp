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
	vint							rowSkip = -1;
	vint							columnSkip = -1;
	vint							rowUntil = -1;
	vint							columnUntil = -1;
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
	vint							rowStart;
	vint							columnStart;
	vint							rowEnd;
	vint							columnEnd;

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
	ParsingArguments				pa;
	Dictionary<WString, Symbol*>	ids;
	IndexMap						index[(vint)IndexReason::Max];
	ReverseIndexMap					reverseIndex[(vint)IndexReason::Max];
	Dictionary<IndexToken, Symbol*>	decls;
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
				result.decls.Add(IndexToken::GetToken(name), symbol);
			}
		}
		for (vint i = 0; i < symbol->declarations.Count(); i++)
		{
			auto& name = symbol->declarations[i]->name;
			if (name.tokenCount > 0)
			{
				result.decls.Add(IndexToken::GetToken(name), symbol);
			}
		}
	}
}

/***********************************************************************
Generating
***********************************************************************/

struct AdjustSkippingResult
{
	vint							rowSkipped = 0;
	vint							rowUntil = 0;
	vint							columnUntil = 0;
};

struct IndexTracking
{
	vint							index = 0;
	bool							inRange = false;
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

void GenerateHtml(Ptr<RegexLexer> lexer, const WString& title, FilePath pathPreprocessed, FilePath pathInput, FilePath pathMapping, IndexResult& result, FilePath pathHtml)
{
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

	WString preprocessed = File(pathPreprocessed).ReadAllTextByBom();
	CppTokenReader reader(lexer, preprocessed, false);
	auto cursor = reader.GetFirstToken();

	{
		FileStream fileStream(pathHtml.GetFullPath(), FileStream::WriteOnly);
		Utf8Encoder encoder;
		EncoderStream encoderStream(fileStream, encoder);
		StreamWriter writer(encoderStream);

		writer.WriteLine(L"<!DOCTYPE html>");
		writer.WriteLine(L"<html>");
		writer.WriteLine(L"<head>");
		writer.WriteLine(L"    <title>" + title + L"</title>");
		writer.WriteLine(L"    <link rel=\"stylesheet\" href=\"../Cpp.css\" />");
		writer.WriteLine(L"    <link rel=\"shortcut icon\" href=\"../favicon.ico\" />");
		writer.WriteLine(L"    <script src=\"../Cpp.js\" ></script>");
		writer.WriteLine(L"</head>");
		writer.WriteLine(L"<body>");
		writer.WriteLine(L"<div class=\"codebox\"><div class=\"cpp_default\">");

		AdjustSkippingResult asr;
		IndexTracking indexSkipping, indexDecl, indexResolve[(vint)IndexReason::Max];
		while (cursor)
		{
			AdjustSkippingIndex(cursor, skipping, indexSkipping, asr);
			if (!indexSkipping.inRange)
			{
				AdjustRefIndex(cursor, result.decls.Keys(), indexDecl, asr);
				for (vint i = 0; i < (vint)IndexReason::Max; i++)
				{
					AdjustRefIndex(cursor, result.index[i].Keys(), indexResolve[i], asr);
				}
			}

			bool isDefToken = indexDecl.inRange;
			bool isRefToken = indexResolve[(vint)IndexReason::Resolved].inRange || indexResolve[(vint)IndexReason::OverloadedResolution].inRange;
			const wchar_t* divClass = nullptr;

			Symbol* symbolForToken = nullptr;
			if (isDefToken)
			{
				symbolForToken = result.decls.Values()[indexDecl.index];
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

			if (isDefToken)
			{
				writer.WriteString(L"<div class=\"def\" id=\"symbol$");
				writer.WriteString(result.decls.Values()[indexDecl.index]->uniqueId);
				writer.WriteString(L"\">");
			}
			if (divClass || isRefToken)
			{
				writer.WriteString(L"<div class=\"");
				if (isRefToken)
				{
					writer.WriteString(L"ref ");
				}
				if (divClass)
				{
					writer.WriteString(L"token ");
					writer.WriteString(divClass);
				}

				for (vint i = (vint)IndexReason::OverloadedResolution; i >= (vint)IndexReason::Resolved; i--)
				{
					if (indexResolve[i].inRange)
					{
						auto& symbols = result.index[i].GetByIndex(indexResolve[i].index);
						writer.WriteString(L"\" onclick=\"jumpToSymbol([");
						for (vint j = 0; j < symbols.Count(); j++)
						{
							if (j != 0) writer.WriteString(L", ");
							writer.WriteString(L"\'");
							writer.WriteString(symbols[j]->uniqueId);
							writer.WriteString(L"\'");
						}
						writer.WriteString(L"])");
						break;
					}
				}
				writer.WriteString(L"\">");
			}
			
			auto reading = cursor->token.reading;
			auto length = cursor->token.length;
			for (vint i = 0; i < length; i++)
			{
				switch (reading[i])
				{
				case L'\r':
					break;
				case L'\n':
					writer.WriteLine(L"");
					break;
				case L'<':
					writer.WriteString(L"&lt;");
					break;
				case L'>':
					writer.WriteString(L"&gt;");
					break;
				case L'&':
					writer.WriteString(L"&amp;");
					break;
				case L'\'':
					writer.WriteString(L"&apos;");
					break;
				case L'\"':
					writer.WriteString(L"&quot;");
					break;
				default:
					writer.WriteChar(reading[i]);
				}
			}

			if (divClass|| isRefToken)
			{
				writer.WriteString(L"</div>");
			}
			if (isDefToken)
			{
				writer.WriteString(L"</div>");
			}
			cursor = cursor->Next();
		}

		writer.WriteLine(L"</div></div>");
		writer.WriteLine(L"</body>");
		writer.WriteLine(L"</html>");
	}
}

/***********************************************************************
Main
***********************************************************************/

int main()
{
	Folder folderCase(L"../UnitTest_Cases");
	List<File> files;
	folderCase.GetFiles(files);
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
			GenerateHtml(
				lexer,
				file.GetFilePath().GetName(),
				pathPreprocessed,
				pathInput,
				pathMapping,
				indexResult,
				folderOutput.GetFilePath() / L"Preprocessed.html"
			);
		}
	}

	return 0;
}