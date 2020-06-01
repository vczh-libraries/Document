#include "Render.h"

/***********************************************************************
TokenTracker
***********************************************************************/

struct AdjustSkippingResult
{
	vint					rowSkipped = 0;		// total rows that are skipped before the current token
	vint					rowUntil = 0;		// the row of the first token after the next skipping
	vint					columnUntil = 0;	// the column of the first token after the next skipping
};

struct IndexTracking
{
	vint					index = 0;
	bool					inRange = false;
};

struct TokenTracker
{
	AdjustSkippingResult	asr;
	IndexTracking			indexSkipping;
	IndexTracking			indexDecl;
	IndexTracking			indexResolve[(vint)IndexReason::Max];
	vint					lastTokenDefIndex = -1;
	vint					lastTokenRefIndex[(vint)IndexReason::Max];

	TokenTracker()
	{
		for (vint i = 0; i < (vint)IndexReason::Max; i++)
		{
			lastTokenRefIndex[i] = -1;
		}
	}

	bool LastTokenRefIndexAllInvalid()
	{
		for (vint i = 0; i < (vint)IndexReason::Max; i++)
		{
			if (lastTokenRefIndex[i] != -1) return false;
		}
		return true;
	}
};

/***********************************************************************
AdjustSkippingIndex
  Check if a token is in any skipped contents
***********************************************************************/

void AdjustSkippingIndex(
	Ptr<CppTokenCursor>& cursor,
	Array<TokenSkipping>& skipping,		// Mapping.bin
	IndexTracking& index,				// TokenTracker::indexSkipping
	AdjustSkippingResult& asr			// TokenTracker::asr
)
{
	auto& token = cursor->token;

	// increase TokenTracker.indexSkipping.index
	while (true)
	{
		if (index.index >= skipping.Count())
		{
			index.inRange = false;
			return;
		}

		// until the whole skipping[index] is not located before the current token
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

	// check if the token belongs to skipping[index] or not
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

void AdjustRefIndex(
	Ptr<CppTokenCursor>& cursor,
	const SortedList<IndexToken>& keys,		// tokens of all declaration names,	or tokens of references
	IndexTracking& index,					// TokenTracker::indexDecl,			or TokenTracker::indexResolve[x]
	const AdjustSkippingResult& asr			// TokenTracker::asr
)
{
	// adjust the row and column of the cursor
	// from Preprocessed.cpp coordination to Index.cpp coordination
	// since indexed token informations are based on Index.cpp, but we are reading tokens from Preprocessed.cpp
	vint row = cursor->token.rowStart;
	vint column = cursor->token.columnStart;
	if (row == asr.rowUntil)
	{
		column -= asr.columnUntil;
	}
	row -= asr.rowSkipped;

	// increase TokenTracker.(indexDecl or indexResolve[x]).index
	while (true)
	{
		if (index.index >= keys.Count())
		{
			index.inRange = false;
			return;
		}

		// until the whole name[index] or reference[index] is not located before the current token
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

	// check if the token belongs to name[index] or reference[index] or not
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

template<typename TCallback>
void GenerateHtmlToken(
	Ptr<CppTokenCursor>& cursor,
	Symbol* symbolForToken,
	const wchar_t*& rawBegin,
	const wchar_t*& rawEnd,
	Ptr<StreamHolder>& html,
	vint& lineCounter,
	TCallback&& callback
)
{
	const wchar_t* divClass = nullptr;

	switch ((CppTokens)cursor->token.token)
	{
		// some tokens has a fixed color
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
	case CppTokens::FLOATHEX:
		divClass = L"cpp_number ";
		break;
#define CASE_KEYWORD(NAME, REGEX) case CppTokens::NAME:
		CPP_KEYWORD_TOKENS(CASE_KEYWORD)
#undef CASE_KEYWORD
			divClass = L"cpp_keyword ";
		break;
	default:
		// others depend on the symbol
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
			// if the current token belongs to any item in skipping
			// then it could not be either a declaration name, or a declaration reference
			tracker.indexDecl.inRange = false;
			for (vint i = 0; i < (vint)IndexReason::Max; i++)
			{
				tracker.indexResolve[i].inRange = false;
			}
		}
		else
		{
			// if the current token does not belong to any item in skipping
			// check if it belongs to a declaration name
			AdjustRefIndex(cursor, result.decls.Keys(), tracker.indexDecl, tracker.asr);
			// check if it belongs to a declaration reference
			for (vint i = 0; i < (vint)IndexReason::Max; i++)
			{
				AdjustRefIndex(cursor, result.index[i].Keys(), tracker.indexResolve[i], tracker.asr);
			}
		}

		// a link is not possible to be the last token of a valid C++ file, so this should just work
		auto flr = global->fileLines[currentFilePath];
		bool isDefToken = tracker.indexDecl.inRange;
		bool isRefToken = tracker.indexResolve[(vint)IndexReason::Resolved].inRange || tracker.indexResolve[(vint)IndexReason::OverloadedResolution].inRange;

		{
			for (vint i = 0; i < (vint)IndexReason::Max; i++)
			{
				if (tracker.lastTokenRefIndex[i] != -1 && (!isRefToken || (tracker.lastTokenRefIndex[i] != tracker.indexResolve[i].index)))
				{
					// if we have gone through all tokens of a declaration reference, close the <div>
					Use(html).WriteString(L"</div>");
					for (vint j = 0; j < (vint)IndexReason::Max; j++)
					{
						tracker.lastTokenRefIndex[j] = -1;
					}
					break;
				}
			}

			if (tracker.lastTokenDefIndex != -1 && (!isDefToken || (tracker.lastTokenDefIndex != tracker.indexDecl.index)))
			{
				// if we have gone through all tokens of a declaration name, close <div>s
				Use(html).WriteString(L"</div></div>");
				tracker.lastTokenDefIndex = -1;
			}
		}

		if (isDefToken && tracker.lastTokenDefIndex == -1)
		{
			// if we hit the first token of a declaration name
			tracker.lastTokenDefIndex = tracker.indexDecl.index;

			// say this declaration belongs to this file
			auto declOrArg = result.decls.Values()[tracker.indexDecl.index];
			if (!global->declToFiles.Keys().Contains(declOrArg))
			{
				global->declToFiles.Add(declOrArg, currentFilePath);
			}

			// generate an id to jump to
			Use(html).WriteString(L"<div class=\"def\" id=\"");
			Use(html).WriteString(GetDeclId(declOrArg));
			Use(html).WriteString(L"\">");

			bool generateForwardImplLink = false;
			Symbol* generatePsLink = nullptr;
			Symbol* generatePrimaryLink = nullptr;

			// no need to generate a link for template argument name
			if (auto& decl = declOrArg.f0)
			{
				switch (decl->symbol->GetCategory())
				{
				case symbol_component::SymbolCategory::Normal:
					// generate a link on a declaration name, if the same symbol appears at multiple places
					generateForwardImplLink = (decl->symbol->GetImplDecl_NFb() ? 1 : 0) + decl->symbol->GetForwardDecls_N().Count() > 1;
					break;
				case symbol_component::SymbolCategory::FunctionBody:
					{
						// generate a link on a function name, if the same symbol appears at multiple places
						auto functionSymbol = decl->symbol->GetFunctionSymbol_Fb();
						generateForwardImplLink = functionSymbol->GetImplSymbols_F().Count() + functionSymbol->GetForwardSymbols_F().Count() > 1;
					}
					break;
				}

				auto declSymbol = decl->symbol;
				if (declSymbol->GetCategory() == symbol_component::SymbolCategory::FunctionBody)
				{
					declSymbol = declSymbol->GetFunctionSymbol_Fb();
				}

				if (declSymbol->IsPSPrimary_NF())
				{
					generatePsLink = declSymbol;
				}
				else
				{
					generatePrimaryLink = declSymbol->GetPSPrimary_NF();
				}
			}


			WString resolvedLinkCode;
			WString psLinkCode;
			WString primaryLinkCode;

			if (generateForwardImplLink)
			{
				auto& decl = declOrArg.f0;
				switch (decl->symbol->GetCategory())
				{
				case symbol_component::SymbolCategory::Normal:
					{
						// if it is not a function declaration, use the declaration symbol
						auto declSymbol = decl->symbol;
						// say we have a hyperlink to this symbol in this file
						if (!flr->refSymbols.Contains(declSymbol))
						{
							flr->refSymbols.Add(declSymbol);
						}
					}
					break;
				case symbol_component::SymbolCategory::FunctionBody:
					{
						// if it is a function declaration, use the function symbol instead of the declaration symbol
						auto declSymbol = decl->symbol->GetFunctionSymbol_Fb();
						// say we have a hyperlink to this symbol in this file
						if (!flr->refSymbols.Contains(declSymbol))
						{
							flr->refSymbols.Add(declSymbol);
						}
					}
					break;
				}

				// generate a <div> to itself, so that users can navigate between forward declarations and implementations
				resolvedLinkCode = L"\'" + GetSymbolId(decl->symbol) + L"\'";
			}

			if (generatePsLink)
			{
				auto& descendants = generatePsLink->GetPSPrimaryDescendants_NF();
				for (vint i = 0; i < descendants.Count(); i++)
				{
					auto psSymbol = descendants[i];

					// say we have a hyperlink to this symbol in this file
					if (!flr->refSymbols.Contains(psSymbol))
					{
						flr->refSymbols.Add(psSymbol);
					}

					psLinkCode += (i > 0 ? L", \'" : L"\'") + GetSymbolId(psSymbol) + L"\'";
				}
			}

			if (generatePrimaryLink)
			{
				// say we have a hyperlink to this symbol in this file
				if (!flr->refSymbols.Contains(generatePrimaryLink))
				{
					flr->refSymbols.Add(generatePrimaryLink);
				}

				primaryLinkCode = L"\'" + GetSymbolId(generatePrimaryLink) + L"\'";
			}

			if (resolvedLinkCode != L"" || psLinkCode != L"" || primaryLinkCode != L"")
			{
				Use(html).WriteString(L"<div class=\"ref\" onclick=\"jumpToSymbol([], [");
				Use(html).WriteString(resolvedLinkCode);
				Use(html).WriteString(L"], [");
				Use(html).WriteString(psLinkCode);
				Use(html).WriteString(L"], [");
				Use(html).WriteString(primaryLinkCode);
				Use(html).WriteString(L"])\">");
			}
			else
			{
				// if this token doesn't need a hyper link
				// generate an empty <div>
				Use(html).WriteString(L"<div>");
			}
		}

		// sometimes the compiler will try to parse an expression and see if it fails.
		// in this case an indexed token may finally become the name of a definition.
		// so we should ignore these tokens.
		if (!isDefToken && isRefToken && tracker.LastTokenRefIndexAllInvalid())
		{
			// if we hit the first token of a declaration reference, and it is not inside a declaration name
			// we definitely need a hyperlink
			for (vint i = 0; i < (vint)IndexReason::Max; i++)
			{
				tracker.lastTokenRefIndex[i] = tracker.indexResolve[i].index;
			}

			for (vint i = (vint)IndexReason::OverloadedResolution; i >= (vint)IndexReason::Resolved; i--)
			{
				if (tracker.indexResolve[i].inRange)
				{
					auto& symbols = result.index[i].GetByIndex(tracker.indexResolve[i].index);
					for (vint j = 0; j < symbols.Count(); j++)
					{
						// say we have a hyperlink to this symbol in this file
						// indexing always give Function instead of FunctionBody
						auto symbol = symbols[j];
						if (symbol->GetCategory() == symbol_component::SymbolCategory::FunctionBody)
						{
							symbol = symbol->GetFunctionSymbol_Fb();
						}
						if (!flr->refSymbols.Contains(symbol))
						{
							flr->refSymbols.Add(symbol);
						}
					}
				}
			}

			// generate a <div> to the target symbol, so that users can navigate between forward declarations and implementations
			Use(html).WriteString(L"<div class=\"ref\" onclick=\"jumpToSymbol(");
			for (vint i = (vint)IndexReason::OverloadedResolution; i >= (vint)IndexReason::Resolved; i--)
			{
				Use(html).WriteString(L"[");

				SortedList<WString> sortedIds;
				if (tracker.indexResolve[i].inRange)
				{
					auto& symbols = result.index[i].GetByIndex(tracker.indexResolve[i].index);
					for (vint j = 0; j < symbols.Count(); j++)
					{
						sortedIds.Add(GetSymbolId(symbols[j]));
					}
				}

				for (vint j = 0; j < sortedIds.Count(); j++)
				{
					if (j != 0) Use(html).WriteString(L", ");
					Use(html).WriteString(L"\'");
					Use(html).WriteString(sortedIds[j]);
					Use(html).WriteString(L"\'");
				}
				Use(html).WriteString(L"], ");
			}
			Use(html).WriteString(L"[], [])\">");
		}

		if (!firstToken && cursor && (CppTokens)cursor->token.token == CppTokens::SHARP)
		{
			// if the current token is #, and it is not the first token to call GenerateHtmlLine
			// it may be exlucded from generated HTML content
			// e.g.
			//   #pragma needs to generate HTML
			//   #line needs not to because it doesn't appear in any source file
			break;
		}

		// choose a color for this token
		Symbol* symbolForToken = nullptr;
		if (isDefToken)
		{
			auto declOrArg = result.decls.Values()[tracker.indexDecl.index];
			symbolForToken = declOrArg.f0 ? declOrArg.f0->symbol : declOrArg.f1;
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

		// generate HTML for this token
		GenerateHtmlToken(cursor, symbolForToken, rawBegin, rawEnd, html, lineCounter, callback);
		SkipToken(cursor);
		firstToken = false;
	}
}

/***********************************************************************
Collect
  Generate HTML for Preprocessed.cpp
  link each line to the original source file
***********************************************************************/

Ptr<GlobalLinesRecord> Collect(
	Ptr<RegexLexer> lexer,
	FilePath pathPreprocessed,				// (in) Preprocessed.cpp
	FilePath pathInput,						// (in) Input.cpp
	FilePath pathMapping,					// (in) Mapping.bin
	IndexResult& result,					// (in) indexing
	IProgressReporter* progressReporter
)
{
	// read Mapping.bin
	auto global = MakePtr<GlobalLinesRecord>();
	Dictionary<WString, FilePath> filePathCache;
	Array<TokenSkipping> skipping;
	ReadMappingFile(pathMapping, skipping);

	// read Preprocessed.cpp, append a line-break if there is no one
	global->preprocessed = File(pathPreprocessed).ReadAllTextByBom();
	if (global->preprocessed.Right(1) != L"\n")
	{
		global->preprocessed += L"\r\n";
	}

	CppTokenReader reader(lexer, global->preprocessed, false);
	auto cursor = reader.GetFirstToken();

	vint currentLineNumber = 0;			// current line number in the source file
	FilePath currentFilePath;			// current file path of the source file
	bool rightAfterSharpLine = false;	// true if the previous line in Preprocessed.cpp is #line
	TokenTracker tracker;

	// generate line-based HTML code for Preprocessed.cpp
	while (cursor)
	{
		{
			// try to consume a #line
			// this line does not exist in any source files before preprocessed
			// so it is not necessary to generate HTML for this line
			auto oldCursor = cursor;
			{
				// #line NUMBER "FILENAME"
				if (!TestToken(cursor, CppTokens::SHARP)) goto GIVE_UP;
				if (!TestToken(cursor, L"line")) goto GIVE_UP;
				if (!TestToken(cursor, CppTokens::SPACE)) goto GIVE_UP;
				if (!TestToken(cursor, CppTokens::INT, false)) goto GIVE_UP;
				vint lineNumber = wtoi(WString(cursor->token.reading, cursor->token.length));
				SkipToken(cursor);
				if (!TestToken(cursor, CppTokens::SPACE)) goto GIVE_UP;
				if (!TestToken(cursor, CppTokens::STRING, false)) goto GIVE_UP;

				// the last "FILENAME" token is not consumed, copy its content, process escaped characters
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

				// the next line is the real NUMBER-th line
				currentLineNumber = lineNumber - 1;
				{
					// convert from "FILENAME" to FilePath
					// read FilePath from a cache if we have already calculated the full name
					// prevent from accessing the file system to normalize the file path
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

				// ensure that a FileLinesRecord object for this source file is created
				{
					vint fileIndex = global->fileLines.Keys().IndexOf(currentFilePath);
					if (fileIndex == -1)
					{
						auto flr = MakePtr<FileLinesRecord>();
						flr->filePath = currentFilePath;

						// set the name of the generated HTML file
						// if there are multiple source files with the same name, add a counter
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

		// if it is not #line, generate HTML for this line
		GenerateHtmlLine(cursor, global, currentFilePath, skipping, result, tracker, [&](HtmlLineRecord hlr)
		{
			if (progressReporter && cursor)
			{
				progressReporter->OnProgress((vint)IIndexRecorder::Phase::Finished, cursor->token.start, global->preprocessed.Length());
			}

			if (rightAfterSharpLine)
			{
				// line break is not consumed after #line
				if (hlr.htmlCode.Length() != 0)
				{
					throw Exception(L"An empty line should have been submitted right after #line.");
				}
				rightAfterSharpLine = false;
			}
			else
			{
				auto flr = global->fileLines[currentFilePath];
				// some header file will be included multiple times
				// ignore them if we generated the same HTML code
				// e.g. pshpack4.h

				vint index = flr->lines.Keys().IndexOf(currentLineNumber);
				if (index == -1)
				{
					flr->lines.Add(currentLineNumber, hlr);
				}
				else
				{
					auto& hlr2 = flr->lines.Values()[index];
					if (hlr.htmlCode != hlr2.htmlCode)
					{
						if (hlr.htmlCode == L"")
						{
							// ignore since the incomming code is empty
						}
						else if (hlr2.htmlCode == L"")
						{
							// replace the previous generated empty code
							flr->lines.Set(currentLineNumber, hlr);
						}
						else
						{
							throw Exception(L"Generated different HTML code for same part of a file.");
						}
					}
				}
				currentLineNumber += hlr.lineCount;
			}
		});
	}

	return global;
}