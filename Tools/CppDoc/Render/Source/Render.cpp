#include "Render.h"

/***********************************************************************
PreprocessedFileToCompactCodeAndMapping
***********************************************************************/

void PreprocessedFileToCompactCodeAndMapping(Ptr<RegexLexer> lexer, FilePath pathInput, FilePath pathPreprocessed, FilePath pathOutput, FilePath pathMapping)
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
										SkipToken(cursor);
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
								SkipToken(cursor);
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
							SkipToken(cursor);
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
				SkipToken(cursor);
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
Compile
***********************************************************************/

void Compile(Ptr<RegexLexer> lexer, FilePath pathFolder, FilePath pathInput, IndexResult& result)
{
	WString input = File(pathInput).ReadAllTextByBom();
	CppTokenReader reader(lexer, input);
	auto cursor = reader.GetFirstToken();

	result.pa = { new Symbol(symbol_component::SymbolCategory::Normal), ITsysAlloc::Create(), new IndexRecorder(result) };
	auto program = ParseProgram(result.pa, cursor);
	EvaluateProgram(result.pa, program);

	result.pa.root->GenerateUniqueId(result.ids, L"");
	for (vint i = 0; i < result.ids.Count(); i++)
	{
		auto symbol = result.ids.Values()[i];
		switch (symbol->GetCategory())
		{
		case symbol_component::SymbolCategory::Normal:
			if (auto decl = symbol->GetImplDecl_NFb())
			{
				auto& name = decl->name;
				if (name.tokenCount > 0)
				{
					result.decls.Add(IndexToken::GetToken(name), decl);
				}
			}
			for (vint i = 0; i < symbol->GetForwardDecls_N().Count(); i++)
			{
				auto decl = symbol->GetForwardDecls_N()[i];
				auto& name = decl->name;
				if (name.tokenCount > 0)
				{
					result.decls.Add(IndexToken::GetToken(name), decl);
				}
			}
			break;
		case symbol_component::SymbolCategory::Function:
			break;
		case symbol_component::SymbolCategory::FunctionBody:
			if (auto decl = symbol->GetImplDecl_NFb())
			{
				auto& name = decl->name;
				if (name.tokenCount > 0)
				{
					result.decls.Add(IndexToken::GetToken(name), decl);
				}
			}
			if (auto decl = symbol->GetForwardDecl_Fb())
			{
				auto& name = decl->name;
				if (name.tokenCount > 0)
				{
					result.decls.Add(IndexToken::GetToken(name), decl);
				}
			}
			break;
		}
	}
}

/***********************************************************************
AdjustSkippingIndex
***********************************************************************/

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

/***********************************************************************
AdjustRefIndex
***********************************************************************/

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
Submit
***********************************************************************/

WString Submit(Ptr<StreamHolder>& holder)
{
	if (!holder) return WString::Empty;
	holder->memoryStream.SeekFromBegin(0);
	auto result = StreamReader(holder->memoryStream).ReadToEnd();
	holder = nullptr;
	return result;
}

/***********************************************************************
Use
***********************************************************************/

StreamWriter& Use(Ptr<StreamHolder>& holder)
{
	if (!holder)
	{
		holder = new StreamHolder;
	}
	return holder->streamWriter;
}

/***********************************************************************
GetSymbolDivClass
***********************************************************************/

const wchar_t* GetSymbolDivClass(Symbol* symbol)
{
	switch (symbol->kind)
	{
	case symbol_component::SymbolKind::Enum:
	case symbol_component::SymbolKind::Class:
	case symbol_component::SymbolKind::Struct:
	case symbol_component::SymbolKind::Union:
	case symbol_component::SymbolKind::TypeAlias:
	case symbol_component::SymbolKind::GenericTypeArgument:
		return L"cpp_type";
	case symbol_component::SymbolKind::EnumItem:
		return L"cpp_enum";
	case symbol_component::SymbolKind::Variable:
		if (auto parent = symbol->GetParentScope())
		{
			if (parent->GetImplDecl_NFb<FunctionDeclaration>())
			{
				return L"cpp_argument";
			}
			else if (parent->GetImplDecl_NFb<ClassDeclaration>())
			{
				return L"cpp_field";
			}
		}
		break;
	case symbol_component::SymbolKind::FunctionBodySymbol:
		return L"cpp_function";
	}
	return nullptr;
}

/***********************************************************************
GenerateHtmlToken
***********************************************************************/

template<typename T>
void GenerateHtmlToken(
	Ptr<CppTokenCursor>& cursor,
	IndexResult& result,
	Symbol* symbolForToken,
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
			divClass = GetSymbolDivClass(symbolForToken);
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
			lineCounter++;
			if (canLineBreak)
			{
				// the last token is always a space, so it is not necessary to submit a line after all cursor is read
				if (rawEnd != &reading[i - 1])
				{
					rawEnd = &reading[i];
				}
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

/***********************************************************************
GenerateHtmlLine
***********************************************************************/

template<typename TCallback>
void GenerateHtmlLine(
	Ptr<CppTokenCursor>& cursor,
	Ptr<GlobalLinesRecord> global,
	FilePath currentFilePath,
	Array<TokenSkipping>& skipping,
	IndexResult& result,
	TokenTracker& tracker,
	TCallback&& callback)
{
	if (!cursor) return;
	const wchar_t* rawBegin = cursor->token.reading;
	const wchar_t * rawEnd = rawBegin;
	Ptr<StreamHolder> html;
	vint lineCounter = 0;

	bool firstToken = true;
	while (cursor)
	{
		// calculate the surrounding context of the current token
		AdjustSkippingIndex(cursor, skipping, tracker.indexSkipping, tracker.asr);
		if (tracker.indexSkipping.inRange)
		{
			tracker.indexDecl.inRange = false;
			for (vint i = 0; i < (vint)IndexReason::Max; i++)
			{
				tracker.indexResolve[i].inRange = false;
			}
		}
		else
		{
			AdjustRefIndex(cursor, result.decls.Keys(), tracker.indexDecl, tracker.asr);
			for (vint i = 0; i < (vint)IndexReason::Max; i++)
			{
				AdjustRefIndex(cursor, result.index[i].Keys(), tracker.indexResolve[i], tracker.asr);
			}
		}

		// a link is not possible to be the last token of a valid C++ file, so this should just work
		auto flr = global->fileLines[currentFilePath];
		bool isDefToken = tracker.indexDecl.inRange;
		bool isRefToken = tracker.indexResolve[(vint)IndexReason::Resolved].inRange || tracker.indexResolve[(vint)IndexReason::OverloadedResolution].inRange;

		if (isDefToken && !tracker.lastTokenIsDef)
		{
			auto decl = result.decls.Values()[tracker.indexDecl.index];
			if (!global->declToFiles.Keys().Contains(decl.Obj()))
			{
				global->declToFiles.Add(decl, currentFilePath);
			}

			Use(html).WriteString(L"<div class=\"def\" id=\"");
			switch (decl->symbol->GetCategory())
			{
			case symbol_component::SymbolCategory::Normal:
				{
					if (decl == decl->symbol->GetImplDecl_NFb())
					{
						Use(html).WriteString(L"NI$");
					}
					else
					{
						Use(html).WriteString(L"NF[" + itow(decl->symbol->GetForwardDecls_N().IndexOf(decl.Obj())) + L"]$");
					}
					Use(html).WriteString(decl->symbol->uniqueId);
				}
				break;
			case symbol_component::SymbolCategory::FunctionBody:
				{
					Use(html).WriteString(L"FB$");
					Use(html).WriteString(decl->symbol->uniqueId);
				}
				break;
			}
			Use(html).WriteString(L"\">");

			bool generateLink = false;
			switch (decl->symbol->GetCategory())
			{
			case symbol_component::SymbolCategory::Normal:
				generateLink = (decl->symbol->GetImplDecl_NFb() ? 1 : 0) + decl->symbol->GetForwardDecls_N().Count() > 1;
				break;
			case symbol_component::SymbolCategory::FunctionBody:
				{
					auto functionSymbol = decl->symbol->GetFunctionSymbol_Fb();
					generateLink = functionSymbol->GetImplSymbols_F().Count() + functionSymbol->GetForwardSymbols_F().Count() > 1;
				}
				break;
			}

			if (generateLink)
			{
				if (!flr->refSymbols.Contains(decl->symbol))
				{
					switch (decl->symbol->GetCategory())
					{
					case symbol_component::SymbolCategory::Normal:
						flr->refSymbols.Add(decl->symbol);
						break;
					case symbol_component::SymbolCategory::FunctionBody:
						flr->refSymbols.Add(decl->symbol->GetFunctionSymbol_Fb());
						break;
					}
				}
				Use(html).WriteString(L"<div class=\"ref\" onclick=\"jumpToSymbol([], [\'");
				switch (decl->symbol->GetCategory())
				{
				case symbol_component::SymbolCategory::Normal:
					Use(html).WriteString(decl->symbol->uniqueId);
					break;
				case symbol_component::SymbolCategory::FunctionBody:
					Use(html).WriteString(decl->symbol->GetFunctionSymbol_Fb()->uniqueId);
					break;
				}
				Use(html).WriteString(L"\'])\">");
			}
			else
			{
				Use(html).WriteString(L"<div>");
			}
		}
		else if (!isDefToken && tracker.lastTokenIsDef)
		{
			Use(html).WriteString(L"</div></div>");
		}

		// sometimes the compiler will try to parse an expression and see if it fails.
		// in this case an indexed token may finally become the name of a definition.
		// so we should ignore these tokens.
		if (!isDefToken && isRefToken && !tracker.lastTokenIsRef)
		{
			for (vint i = (vint)IndexReason::OverloadedResolution; i >= (vint)IndexReason::Resolved; i--)
			{
				if (tracker.indexResolve[i].inRange)
				{
					auto& symbols = result.index[i].GetByIndex(tracker.indexResolve[i].index);
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

			Use(html).WriteString(L"<div class=\"ref\" onclick=\"jumpToSymbol(");
			for (vint i = (vint)IndexReason::OverloadedResolution; i >= (vint)IndexReason::Resolved; i--)
			{
				if (i != (vint)IndexReason::OverloadedResolution)
				{
					Use(html).WriteString(L", ");
				}
				Use(html).WriteString(L"[");
				if (tracker.indexResolve[i].inRange)
				{
					auto& symbols = result.index[i].GetByIndex(tracker.indexResolve[i].index);
					for (vint j = 0; j < symbols.Count(); j++)
					{
						if (j != 0) Use(html).WriteString(L", ");
						Use(html).WriteString(L"\'");
						{
							auto symbol = symbols[j];
							switch (symbol->GetCategory())
							{
							case symbol_component::SymbolCategory::Normal:
							case symbol_component::SymbolCategory::Function:
								Use(html).WriteString(symbol->uniqueId);
								break;
							case symbol_component::SymbolCategory::FunctionBody:
								Use(html).WriteString(symbol->GetFunctionSymbol_Fb()->uniqueId);
								break;
							}
						}
						Use(html).WriteString(L"\'");
					}
				}
				Use(html).WriteString(L"]");
			}
			Use(html).WriteString(L")\">");
		}
		else if (!tracker.lastTokenIsDef && !isRefToken && tracker.lastTokenIsRef)
		{
			Use(html).WriteString(L"</div>");
		}

		tracker.lastTokenIsDef = isDefToken;
		tracker.lastTokenIsRef = isRefToken;

		if (!firstToken && cursor && (CppTokens)cursor->token.token == CppTokens::SHARP)
		{
			// let the outside decide whether this # need to be generate HTML code or not
			break;
		}

		// write a token
		Symbol* symbolForToken = nullptr;
		if (isDefToken)
		{
			symbolForToken = result.decls.Values()[tracker.indexDecl.index]->symbol;
		}
		else if (isRefToken)
		{
			for (vint i = (vint)IndexReason::OverloadedResolution; i >= (vint)IndexReason::Resolved; i--)
			{
				if (tracker.indexResolve[i].inRange)
				{
					symbolForToken = result.index[i].GetByIndex(tracker.indexResolve[i].index)[0];
					break;
				}
			}
		}
		GenerateHtmlToken(cursor, result, symbolForToken, rawBegin, rawEnd, html, lineCounter, callback);
		SkipToken(cursor);
		firstToken = false;
	}
}

/***********************************************************************
Collect
***********************************************************************/

Ptr<GlobalLinesRecord> Collect(Ptr<RegexLexer> lexer, const WString& title, FilePath pathPreprocessed, FilePath pathInput, FilePath pathMapping, IndexResult& result)
{
	auto global = MakePtr<GlobalLinesRecord>();
	Dictionary<WString, FilePath> filePathCache;
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
	FilePath currentFilePath;
	bool rightAfterSharpLine = false;
	TokenTracker tracker;
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
				{
					WString filePathText(&buffer[0], (vint)(writing - &buffer[0]));
					vint index = filePathCache.Keys().IndexOf(filePathText);
					if (index == -1)
					{
						currentFilePath = filePathText;
						filePathCache.Add(filePathText, currentFilePath);
					}
					else
					{
						currentFilePath = filePathCache.Values()[index];
					}
				}

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
							flr->htmlFileName = displayName + (counter == 1 ? WString::Empty : itow(counter));
							if (!global->htmlFileNames.Contains(flr->htmlFileName))
							{
								global->htmlFileNames.Add(flr->htmlFileName);
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
		GenerateHtmlLine(cursor, global, currentFilePath, skipping, result, tracker, [&](HtmlLineRecord hlr)
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
Source Code Page Generating
***********************************************************************/

WString GetDisplayNameInHtml(Symbol* symbol);

class GetDisplayNameInHtmlTypeVisitor : public Object, public ITypeVisitor
{
public:
	WString											result;

	void Visit(PrimitiveType* self)override
	{
		switch (self->primitive)
		{
		case CppPrimitiveType::_auto:			result = L"<span class=\"cpp_keyword\">auto</span>" + result;			break;
		case CppPrimitiveType::_void:			result = L"<span class=\"cpp_keyword\">void</span>" + result;			break;
		case CppPrimitiveType::_bool:			result = L"<span class=\"cpp_keyword\">bool</span>" + result;			break;
		case CppPrimitiveType::_char:			result = L"<span class=\"cpp_keyword\">char</span>" + result;			break;
		case CppPrimitiveType::_wchar_t:		result = L"<span class=\"cpp_keyword\">wchar_t</span>" + result;		break;
		case CppPrimitiveType::_char16_t:		result = L"<span class=\"cpp_keyword\">char16_t</span>" + result;		break;
		case CppPrimitiveType::_char32_t:		result = L"<span class=\"cpp_keyword\">char32_t</span>" + result;		break;
		case CppPrimitiveType::_short:			result = L"<span class=\"cpp_keyword\">short</span>" + result;			break;
		case CppPrimitiveType::_int:			result = L"<span class=\"cpp_keyword\">int</span>" + result;			break;
		case CppPrimitiveType::___int8:			result = L"<span class=\"cpp_keyword\">__int8</span>" + result;			break;
		case CppPrimitiveType::___int16:		result = L"<span class=\"cpp_keyword\">__int16</span>" + result;		break;
		case CppPrimitiveType::___int32:		result = L"<span class=\"cpp_keyword\">__int32</span>" + result;		break;
		case CppPrimitiveType::___int64:		result = L"<span class=\"cpp_keyword\">__int64;</span>" + result;		break;
		case CppPrimitiveType::_long:			result = L"<span class=\"cpp_keyword\">long</span>" + result;			break;
		case CppPrimitiveType::_long_int:		result = L"<span class=\"cpp_keyword\">long int</span>" + result;		break;
		case CppPrimitiveType::_long_long:		result = L"<span class=\"cpp_keyword\">long long</span>" + result;		break;
		case CppPrimitiveType::_float:			result = L"<span class=\"cpp_keyword\">float</span>" + result;			break;
		case CppPrimitiveType::_double:			result = L"<span class=\"cpp_keyword\">double</span>" + result;			break;
		case CppPrimitiveType::_long_double:	result = L"<span class=\"cpp_keyword\">long double</span>" + result;	break;
		}

		switch (self->prefix)
		{
		case CppPrimitivePrefix::_signed:		result = L"<span class=\"cpp_keyword\">signed</span>" + result;			break;
		case CppPrimitivePrefix::_unsigned:		result = L"<span class=\"cpp_keyword\">unsigned</span>" + result;		break;
		}
	}

	void Visit(ReferenceType* self)override
	{
		switch (self->reference)
		{
		case CppReferenceType::Ptr:				result = L" *" + result;	break;
		case CppReferenceType::LRef:			result = L" &" + result;	break;
		case CppReferenceType::RRef:			result = L" &&" + result;	break;
		}
		self->type->Accept(this);
	}

	void Visit(ArrayType* self)override
	{
		if (result != L"")
		{
			result = L"(" + result + L")";
		}

		if (self->expr)
		{
			result += L"[...]";
		}
		else
		{
			result += L"[]";
		}

		self->type->Accept(this);
	}

	void Visit(CallingConventionType* self)override
	{
		switch (self->callingConvention)
		{
		case TsysCallingConvention::CDecl:		result = L" <span class=\"cpp_keyword\">__cdecl</span>" + result;		break;
		case TsysCallingConvention::ClrCall:	result = L" <span class=\"cpp_keyword\">__clrcall</span>" + result;		break;
		case TsysCallingConvention::StdCall:	result = L" <span class=\"cpp_keyword\">__stdcall</span>" + result;		break;
		case TsysCallingConvention::FastCall:	result = L" <span class=\"cpp_keyword\">__fastcall</span>" + result;	break;
		case TsysCallingConvention::ThisCall:	result = L" <span class=\"cpp_keyword\">__thiscall</span>" + result;	break;
		case TsysCallingConvention::VectorCall:	result = L" <span class=\"cpp_keyword\">__vectorcall</span>" + result;	break;
		}
		self->type->Accept(this);
	}

	void Visit(FunctionType* self)override
	{
		if (result != L"")
		{
			result = L"(" + result + L")";
		}

		result += L"(";
		for (vint i = 0; i < self->parameters.Count(); i++)
		{
			if (i != 0) result += L", ";
			GetDisplayNameInHtmlTypeVisitor visitor;
			self->parameters[i].item->type->Accept(&visitor);
			if (self->parameters[i].isVariadic)
			{
				result += L"...";
			}
			result += visitor.result;
		}
		if (self->ellipsis)
		{
			result += L" ...";
		}
		result += L")";

		if (self->decoratorReturnType)
		{
			result += L"->";
			GetDisplayNameInHtmlTypeVisitor visitor;
			self->decoratorReturnType->Accept(&visitor);
			result += visitor.result;
		}

		self->returnType->Accept(this);
	}

	void Visit(MemberType* self)override
	{
		result = L"::" + result;
		self->classType->Accept(this);
		result = L" " + result;
		self->type->Accept(this);
	}

	void Visit(DeclType* self)override
	{
		if (self->expr)
		{
			result = L"<span class=\"cpp_keyword\">decltype</span>(...)" + result;
		}
		else
		{
			result = L"<span class=\"cpp_keyword\">decltype</span>(<span class=\"cpp_keyword\">auto</span>)" + result;
		}
	}

	void Visit(DecorateType* self)override
	{
		if (self->isVolatile) result = L"<span class=\"cpp_keyword\"> volatile</span>" + result;
		if (self->isConst) result = L"<span class=\"cpp_keyword\"> const</span>" + result;
		if (self->isConstExpr) result = L"<span class=\"cpp_keyword\"> constexpr</span>" + result;
		self->type->Accept(this);
	}

	void Visit(RootType* self)override
	{
		result = L"::" + result;
	}

	void Visit(IdType* self)override
	{
		if (self->resolving)
		{
			result = GetDisplayNameInHtml(self->resolving->resolvedSymbols[0]) + result;
		}
		else
		{
			result = self->name.name + result;
		}
	}

	void Visit(ChildType* self)override
	{
		if (self->resolving)
		{
			result = GetDisplayNameInHtml(self->resolving->resolvedSymbols[0]) + result;
		}
		else
		{
			result = L"::" + self->name.name + result;
			self->classType->Accept(this);
		}
	}

	void Visit(GenericType* self)override
	{
		WString currentResult = result;
		WString genericResult;

		{
			result = L"";
			self->type->Accept(this);
			genericResult = result + L"<";
		}
		for (vint i = 0; i < self->arguments.Count(); i++)
		{
			if (i > 0) genericResult += L", ";
			auto argument = self->arguments[i];
			if (argument.item.type)
			{
				result = L"";
				argument.item.type->Accept(this);
				genericResult += result;
			}
			else
			{
				genericResult += L"...";
			}
		}
		genericResult += L">";

		result = genericResult + L" " + currentResult;
	}
};

WString GetDisplayNameInHtml(Symbol* symbol)
{
	WString displayNameInHtml;
	if (symbol->kind == symbol_component::SymbolKind::Namespace)
	{
		auto current = symbol;
		while (current->kind != symbol_component::SymbolKind::Root)
		{
			if (current != symbol) displayNameInHtml = L"::" + displayNameInHtml;
			displayNameInHtml = current->name + displayNameInHtml;
			current = current->GetParentScope();
		}
	}
	else if (symbol->kind == symbol_component::SymbolKind::Variable && (symbol->GetParentScope() && !symbol->GetParentScope()->GetImplDecl_NFb<ClassDeclaration>()))
	{
		displayNameInHtml = symbol->name;
	}
	else
	{
		auto current = symbol;
		while (current->kind != symbol_component::SymbolKind::Root && current->kind != symbol_component::SymbolKind::Namespace)
		{
			if (current != symbol) displayNameInHtml = L"::" + displayNameInHtml;
			if (auto divClass = GetSymbolDivClass(current))
			{
				displayNameInHtml = L"<span class=\"" + WString(divClass, false) + L"\">" + current->name + L"</span>" + displayNameInHtml;
			}
			else
			{
				displayNameInHtml = current->name + displayNameInHtml;
			}
			current = current->GetParentScope();
		}
	}

	if (auto funcDecl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
	{
		if (auto funcType = GetTypeWithoutMemberAndCC(funcDecl->type).Cast<FunctionType>())
		{
			displayNameInHtml += L"(";
			for (vint i = 0; i < funcType->parameters.Count(); i++)
			{
				if (i != 0) displayNameInHtml += L", ";
				GetDisplayNameInHtmlTypeVisitor visitor;
				funcType->parameters[i].item->type->Accept(&visitor);
				displayNameInHtml += visitor.result;
				if (funcType->parameters[i].isVariadic)
				{
					displayNameInHtml += L"...";
				}
			}
			if (funcType->ellipsis)
			{
				displayNameInHtml += L" ...";
			}
			displayNameInHtml += L")";
		}
	}
	return displayNameInHtml;
}

/***********************************************************************
GenerateFile
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
	writer.WriteLine(L"<a class=\"button\" href=\"./FileIndex.html\">File Index</a>");
	writer.WriteLine(L"<a class=\"button\" href=\"./SymbolIndex.html\">Symbol Index</a>");
	writer.WriteLine(L"<br>");
	writer.WriteLine(L"<br>");
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

					bool rawCodeMatched = (nextProcessingLine - originalIndex) >= currentHtmlLines.lineCount;
					if (rawCodeMatched)
					{
						StringReader reader(WString(currentHtmlLines.rawBegin, (vint)(currentHtmlLines.rawEnd - currentHtmlLines.rawBegin)));
						for (vint i = 0; i < currentHtmlLines.lineCount; i++)
						{
							if (originalLines[originalIndex + i] != reader.ReadLine())
							{
								rawCodeMatched = false;
								break;
							}
						}
					}

					if (rawCodeMatched)
					{
						writer.WriteLine(currentHtmlLines.htmlCode);
						nextProcessingLine = originalIndex + currentHtmlLines.lineCount;
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
				bool hasEmbeddedHtml = embedHtmlInDisabled.Length() != 0;
				if (hasEmbeddedHtml)
				{
					writer.WriteString(L"<div class=\"expandable\"/>");
				}
				writer.WriteString(L"<div class=\"disabled\"/>");
				for (vint i = originalIndex; i < disableEnd; i++)
				{
					if (i > originalIndex)
					{
						writer.WriteLine(L"");
					}
					auto reading = originalLines[i].Buffer();
					while (auto c = *reading++)
					{
						switch (c)
						{
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
							writer.WriteChar(c);
						}
					}
				}
				writer.WriteLine(L"</div>");
				if (hasEmbeddedHtml)
				{
					writer.WriteString(L"<div class=\"expanded\">");
					writer.WriteString(embedHtmlInDisabled);
					writer.WriteLine(L"</div></div>");
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
		writer.WriteLine(L"\': {");

		writer.WriteString(L"        \'displayNameInHtml\': \'");
		writer.WriteString(GetDisplayNameInHtml(symbol));
		writer.WriteLine(L"\',");

		List<WString> impls, decls;
		switch (symbol->GetCategory())
		{
		case symbol_component::SymbolCategory::Normal:
			{
				if (symbol->GetImplDecl_NFb())
				{
					impls.Add(L"NI$" + symbol->uniqueId);
				}
				for (vint j = 0; j < symbol->GetForwardDecls_N().Count(); j++)
				{
					decls.Add(L"NF[" + itow(j) + L"]$" + symbol->uniqueId);
				}
			}
			break;
		case symbol_component::SymbolCategory::Function:
			{
				auto& symbols = symbol->GetImplSymbols_F();
				for (vint j = 0; j < symbols.Count(); j++)
				{
					impls.Add(L"FB$" + symbols[j]->uniqueId);
				}
			}
			{
				auto& symbols = symbol->GetForwardSymbols_F();
				for (vint j = 0; j < symbols.Count(); j++)
				{
					decls.Add(L"FB$" + symbols[j]->uniqueId);
				}
			}
			break;
		case symbol_component::SymbolCategory::FunctionBody:
			throw UnexpectedSymbolCategoryException();
		}

		if (impls.Count() == 0)
		{
			writer.WriteLine(L"        \'impls\': [],");
		}
		else
		{
			writer.WriteLine(L"        \'impls\': [");
			for (vint j = 0; j < impls.Count(); j++)
			{
				writer.WriteString(L"            \'");
				writer.WriteString(impls[j]);
				if (j == impls.Count() - 1)
				{
					writer.WriteLine(L"\'");
				}
				else
				{
					writer.WriteLine(L"\',");
				}
			}
			writer.WriteLine(L"        ],");
		}

		if (decls.Count() == 0)
		{
			writer.WriteLine(L"        \'decls\': []");
		}
		else
		{
			writer.WriteLine(L"        \'decls\': [");
			for (vint j = 0; j < decls.Count(); j++)
			{
				writer.WriteString(L"            \'");
				writer.WriteString(decls[j]);
				if (j == decls.Count() - 1)
				{
					writer.WriteLine(L"\'");
				}
				else
				{
					writer.WriteLine(L"\',");
				}
			}
			writer.WriteLine(L"        ]");
		}

		if (i == flr->refSymbols.Count() - 1)
		{
			writer.WriteLine(L"    }");
		}
		else
		{
			writer.WriteLine(L"    },");
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
		writer.WriteString(L"\': ");

		auto filePath = global->declToFiles.Values()[index];
		auto flrTarget = global->fileLines[filePath];
		if (flrTarget == flr)
		{
			writer.WriteString(L"null");
		}
		else
		{
			writer.WriteString(L"{ \'htmlFileName\': \'");
			writer.WriteString(flrTarget->htmlFileName);
			writer.WriteString(L"\', \'displayName\': \'");
			writer.WriteString(flrTarget->filePath.GetName());
			writer.WriteString(L"\' }");
		}
	};
	for (vint i = 0; i < flr->refSymbols.Count(); i++)
	{
		auto symbol = flr->refSymbols[i];
		switch (symbol->GetCategory())
		{
		case symbol_component::SymbolCategory::Normal:
			{
				if (auto decl = symbol->GetImplDecl_NFb())
				{
					writeFileMapping(L"NI$", decl);
				}
				for (vint j = 0; j < symbol->GetForwardDecls_N().Count(); j++)
				{
					auto decl = symbol->GetForwardDecls_N()[j];
					writeFileMapping(L"NF[" + itow(j) + L"]$", decl);
				}
			}
			break;
		case symbol_component::SymbolCategory::Function:
			{
				auto& symbols = symbol->GetImplSymbols_F();
				for (vint j = 0; j < symbols.Count(); j++)
				{
					writeFileMapping(L"FB$", symbols[j]->GetImplDecl_NFb());
				}
			}
			{
				auto& symbols = symbol->GetForwardSymbols_F();
				for (vint j = 0; j < symbols.Count(); j++)
				{
					writeFileMapping(L"FB$", symbols[j]->GetForwardDecl_Fb());
				}
			}
			break;
		case symbol_component::SymbolCategory::FunctionBody:
			throw UnexpectedSymbolCategoryException();
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
GenerateFileIndex
***********************************************************************/

void GenerateFileIndex(Ptr<GlobalLinesRecord> global, FilePath pathHtml, FileGroupConfig& fileGroups)
{
	FileStream fileStream(pathHtml.GetFullPath(), FileStream::WriteOnly);
	Utf8Encoder encoder;
	EncoderStream encoderStream(fileStream, encoder);
	StreamWriter writer(encoderStream);

	writer.WriteLine(L"<!DOCTYPE html>");
	writer.WriteLine(L"<html>");
	writer.WriteLine(L"<head>");
	writer.WriteLine(L"    <title>File Index</title>");
	writer.WriteLine(L"    <link rel=\"stylesheet\" href=\"../Cpp.css\" />");
	writer.WriteLine(L"    <link rel=\"shortcut icon\" href=\"../favicon.ico\" />");
	writer.WriteLine(L"</head>");
	writer.WriteLine(L"<body>");
	writer.WriteLine(L"<a class=\"button\" href=\"./FileIndex.html\">File Index</a>");
	writer.WriteLine(L"<a class=\"button\" href=\"./SymbolIndex.html\">Symbol Index</a>");
	writer.WriteLine(L"<br>");
	writer.WriteLine(L"<br>");

	List<Ptr<FileLinesRecord>> flrs;
	CopyFrom(
		flrs,
		From(global->fileLines.Values())
		.OrderBy([](Ptr<FileLinesRecord> flr1, Ptr<FileLinesRecord> flr2)
	{
		return WString::Compare(flr1->filePath.GetFullPath(), flr2->filePath.GetFullPath());
	})
	);
	for (vint i = 0; i < fileGroups.Count(); i++)
	{
		auto prefix = fileGroups[i].f0;
		writer.WriteString(L"<span class=\"fileGroupLabel\">");
		writer.WriteString(fileGroups[i].f1);
		writer.WriteLine(L"</span><br>");

		for (vint j = 0; j < flrs.Count(); j++)
		{
			auto flr = flrs[j];
			if (INVLOC.StartsWith(wupper(flr->filePath.GetFullPath()), prefix, Locale::Normalization::None))
			{
				writer.WriteString(L"&nbsp;&nbsp;&nbsp;&nbsp;<a class=\"fileIndex\" href=\"./");
				writer.WriteString(flr->htmlFileName);
				writer.WriteString(L".html\">");
				writer.WriteString(flr->filePath.GetFullPath().Right(flr->filePath.GetFullPath().Length() - prefix.Length()));
				writer.WriteLine(L"</a><br>");
				flrs.RemoveAt(j--);
			}
		}
	}
	{
		writer.WriteString(L"<span class=\"fileGroupLabel\">");

		for (vint j = 0; j < flrs.Count(); j++)
		{
			auto flr = flrs[j];
			writer.WriteString(L"<a class=\"fileIndex\" href=\"./");
			writer.WriteString(flr->htmlFileName);
			writer.WriteString(L".html\">");
			writer.WriteString(flr->filePath.GetFullPath());
			writer.WriteLine(L"</a><br>");
		}
	}
	writer.WriteLine(L"</body>");
	writer.WriteLine(L"</html>");
}

/***********************************************************************
GenerateSymbolIndex
***********************************************************************/

void GenerateSymbolIndex(Ptr<GlobalLinesRecord> global, StreamWriter& writer, const WString& fileGroupPrefix, vint indentation, Symbol* context, bool printBraceBeforeFirstChild, bool& printedChild)
{
	if (context->kind != symbol_component::SymbolKind::Root)
	{
		bool definedInThisFileGroup = false;
		List<Ptr<Declaration>> decls;
		switch (context->GetCategory())
		{
		case symbol_component::SymbolCategory::Normal:
			if (auto decl = context->GetImplDecl_NFb())
			{
				decls.Add(decl);
			}
			for (vint i = 0; i < context->GetForwardDecls_N().Count(); i++)
			{
				decls.Add(context->GetForwardDecls_N()[i]);
			}
			break;
		case symbol_component::SymbolCategory::Function:
			for (vint i = 0; i < context->GetImplSymbols_F().Count(); i++)
			{
				if (auto decl = context->GetImplSymbols_F()[i]->GetImplDecl_NFb())
				{
					decls.Add(decl);
				}
			}
			for (vint i = 0; i < context->GetForwardSymbols_F().Count(); i++)
			{
				if (auto decl = context->GetForwardSymbols_F()[i]->GetForwardDecl_Fb())
				{
					decls.Add(decl);
				}
			}
			break;
		case symbol_component::SymbolCategory::FunctionBody:
			throw UnexpectedSymbolCategoryException();
		}

		for (vint i = 0; i < decls.Count(); i++)
		{
			vint index = global->declToFiles.Keys().IndexOf(decls[i].Obj());
			if (index != -1)
			{
				if (INVLOC.StartsWith(wupper(global->declToFiles.Values()[index].GetFullPath()), fileGroupPrefix, Locale::Normalization::None))
				{
					definedInThisFileGroup = true;
					break;
				}
			}
		}

		if (!definedInThisFileGroup)
		{
			return;
		}
	}

	bool isRoot = false;
	bool searchForChild = false;
	const wchar_t* keyword = nullptr;
	const wchar_t* tokenClass = nullptr;
	switch (context->kind)
	{
	case symbol_component::SymbolKind::Enum:
		if (context->GetAnyForwardDecl<ForwardEnumDeclaration>()->name.tokenCount > 0)
		{
			searchForChild = true;
			if (context->GetAnyForwardDecl<ForwardEnumDeclaration>()->enumClass)
			{
				keyword = L"enum class";
			}
			else
			{
				keyword = L"enum";
			}
			tokenClass = L"cpp_type";
		}
		break;
	case symbol_component::SymbolKind::Class:
		if (context->GetAnyForwardDecl<ForwardClassDeclaration>()->name.tokenCount > 0)
		{
			searchForChild = true;
			keyword = L"class";
			tokenClass = L"cpp_type";
		}
		break;
	case symbol_component::SymbolKind::Struct:
		if (context->GetAnyForwardDecl<ForwardClassDeclaration>()->name.tokenCount > 0)
		{
			searchForChild = true;
			keyword = L"struct";
			tokenClass = L"cpp_type";
		}
		break;
	case symbol_component::SymbolKind::Union:
		if (context->GetAnyForwardDecl<ForwardClassDeclaration>()->name.tokenCount > 0)
		{
			searchForChild = true;
			keyword = L"union";
			tokenClass = L"cpp_type";
		}
		break;
	case symbol_component::SymbolKind::TypeAlias:
		keyword = L"typedef";
		tokenClass = L"cpp_type";
		break;
	case symbol_component::SymbolKind::FunctionSymbol:
		if (!context->GetAnyForwardDecl<ForwardFunctionDeclaration>()->implicitlyGeneratedMember)
		{
			keyword = L"function";
			tokenClass = L"cpp_function";
		}
		break;
	case symbol_component::SymbolKind::Variable:
		keyword = L"variable";
		if (auto parent = context->GetParentScope())
		{
			if (parent->GetImplDecl_NFb<ClassDeclaration>())
			{
				tokenClass = L"cpp_field";
			}
		}
		break;
	case symbol_component::SymbolKind::Namespace:
		searchForChild = true;
		keyword = L"namespace";
		break;
	case symbol_component::SymbolKind::Root:
		searchForChild = true;
		isRoot = true;
		break;
	}

	if (keyword)
	{
		if (printBraceBeforeFirstChild)
		{
			if (!printedChild)
			{
				printedChild = true;
				for (vint i = 0; i < indentation - 1; i++)
				{
					writer.WriteString(L"    ");
				}
				writer.WriteLine(L"{");
			}
		}

		for (vint i = 0; i < indentation; i++)
		{
			writer.WriteString(L"    ");
		}
		writer.WriteString(L"<div class=\"cpp_keyword\">");
		writer.WriteString(keyword);
		writer.WriteString(L"</div>");
		writer.WriteChar(L' ');
		if (tokenClass)
		{
			writer.WriteString(L"<div class=\"");
			writer.WriteString(tokenClass);
			writer.WriteString(L"\">");
		}
		writer.WriteString(context->name);
		if (tokenClass)
		{
			writer.WriteString(L"</div>");
		}

		auto writeTag = [&](const WString& prefix, const WString& tag, Symbol* context, Ptr<Declaration> decl)
		{
			vint index = global->declToFiles.Keys().IndexOf(decl.Obj());
			if (index != -1)
			{
				auto filePath = global->declToFiles.Values()[index];
				auto htmlFileName = global->fileLines[filePath]->htmlFileName;
				writer.WriteString(L"<a class=\"symbolIndex\" href=\"./");
				writer.WriteString(htmlFileName);
				writer.WriteString(L".html#");
				writer.WriteString(prefix);
				writer.WriteString(context->uniqueId);
				writer.WriteString(L"\">");
				writer.WriteString(tag);
				writer.WriteString(L"</a>");
			}
		};

		switch (context->GetCategory())
		{
		case symbol_component::SymbolCategory::Normal:
			{
				if (auto decl = context->GetImplDecl_NFb())
				{
					writeTag(L"NI$", L"impl", context, decl);
				}
				for (vint j = 0; j < context->GetForwardDecls_N().Count(); j++)
				{
					auto decl = context->GetForwardDecls_N()[j];
					writeTag(L"NF[" + itow(j) + L"]$", L"decl", context, decl);
				}
			}
			break;
		case symbol_component::SymbolCategory::Function:
			{
				auto& symbols = context->GetImplSymbols_F();
				for (vint j = 0; j < symbols.Count(); j++)
				{
					auto symbol = symbols[j].Obj();
					writeTag(L"FB$", L"impl", symbol, symbol->GetImplDecl_NFb());
				}
			}
			{
				auto& symbols = context->GetForwardSymbols_F();
				for (vint j = 0; j < symbols.Count(); j++)
				{
					auto symbol = symbols[j].Obj();
					writeTag(L"FB$", L"decl", symbol, symbol->GetForwardDecl_Fb());
				}
			}
			break;
		case symbol_component::SymbolCategory::FunctionBody:
			throw UnexpectedSymbolCategoryException();
		}
		writer.WriteLine(L"");
	}

	if (searchForChild)
	{
		bool printedChildNextLevel = false;
		for (vint i = 0; i < context->GetChildren_NFb().Count(); i++)
		{
			auto& children = context->GetChildren_NFb().GetByIndex(i);
			for (vint j = 0; j < children.Count(); j++)
			{
				GenerateSymbolIndex(global, writer, fileGroupPrefix, indentation + 1, children[j].Obj(), !isRoot, printedChildNextLevel);
			}
		}
		if (printedChildNextLevel)
		{
			for (vint i = 0; i < indentation; i++)
			{
				writer.WriteString(L"    ");
			}
			writer.WriteLine(L"}");
		}
	}
}

void GenerateSymbolIndex(Ptr<GlobalLinesRecord> global, IndexResult& result, FilePath pathHtml, FileGroupConfig& fileGroups)
{
	FileStream fileStream(pathHtml.GetFullPath(), FileStream::WriteOnly);
	Utf8Encoder encoder;
	EncoderStream encoderStream(fileStream, encoder);
	StreamWriter writer(encoderStream);

	writer.WriteLine(L"<!DOCTYPE html>");
	writer.WriteLine(L"<html>");
	writer.WriteLine(L"<head>");
	writer.WriteLine(L"    <title>Symbol Index</title>");
	writer.WriteLine(L"    <link rel=\"stylesheet\" href=\"../Cpp.css\" />");
	writer.WriteLine(L"    <link rel=\"shortcut icon\" href=\"../favicon.ico\" />");
	writer.WriteLine(L"</head>");
	writer.WriteLine(L"<body>");
	writer.WriteLine(L"<a class=\"button\" href=\"./FileIndex.html\">File Index</a>");
	writer.WriteLine(L"<a class=\"button\" href=\"./SymbolIndex.html\">Symbol Index</a>");
	writer.WriteLine(L"<br>");
	writer.WriteLine(L"<br>");
	writer.WriteString(L"<div class=\"codebox\"><div class=\"cpp_default\">");
	for (vint i = 0; i < fileGroups.Count(); i++)
	{
		auto prefix = fileGroups[i].f0;
		writer.WriteString(L"<span class=\"fileGroupLabel\">");
		writer.WriteString(fileGroups[i].f1);
		writer.WriteLine(L"</span>");

		bool printedChild = false;
		GenerateSymbolIndex(global, writer, prefix, 0, result.pa.root.Obj(), false, printedChild);
	}
	writer.WriteLine(L"</div></div>");
	writer.WriteLine(L"</body>");
	writer.WriteLine(L"</html>");
}