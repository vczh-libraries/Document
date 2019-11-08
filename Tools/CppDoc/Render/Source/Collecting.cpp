#include "Render.h"

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
GenerateHtmlToken
***********************************************************************/

template<typename T>
void GenerateHtmlToken(
	Ptr<CppTokenCursor>& cursor,
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
		Use(html).WriteString(L"<div class=\"token ");
		Use(html).WriteString(divClass);
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
			Use(html).WriteString(GetDeclId(decl));
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
						{
							auto declSymbol = decl->symbol;
							if (!flr->refSymbols.Contains(declSymbol))
							{
								flr->refSymbols.Add(declSymbol);
							}
						}
						break;
					case symbol_component::SymbolCategory::FunctionBody:
						{
							auto declSymbol = decl->symbol->GetFunctionSymbol_Fb();
							if (!flr->refSymbols.Contains(declSymbol))
							{
								flr->refSymbols.Add(declSymbol);
							}
						}
						break;
					}
				}
				Use(html).WriteString(L"<div class=\"ref\" onclick=\"jumpToSymbol([], [\'");
				Use(html).WriteString(GetSymbolId(decl->symbol));
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
						Use(html).WriteString(GetSymbolId(symbols[j]));
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
		GenerateHtmlToken(cursor, symbolForToken, rawBegin, rawEnd, html, lineCounter, callback);
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
	ReadMappingFile(pathMapping, skipping);

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