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

template<typename TContainer, typename TGetter>
void AdjustRefIndexInternal(
	Ptr<CppTokenCursor>& cursor,
	const TContainer& tokens,			// tokens of all declaration names,	or tokens of references
	IndexTracking& index,				// TokenTracker::indexDecl,			or TokenTracker::indexResolve[x]
	const AdjustSkippingResult& asr,	// TokenTracker::asr
	TGetter&& getter
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
		if (index.index >= tokens.Count())
		{
			index.inRange = false;
			return;
		}

		// until the whole name[index] or reference[index] is not located before the current token
		const auto& current = getter(tokens, index.index);
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
	const auto& current = getter(tokens, index.index);
	if (row < current.rowStart || (row == current.rowStart && column < current.columnStart))
	{
		index.inRange = false;
		return;
	}

	index.inRange = true;
}

void AdjustRefIndex(
	Ptr<CppTokenCursor>& cursor,
	const List<IndexedDeclOrArg>& decls,
	IndexTracking& index,
	const AdjustSkippingResult& asr
)
{
	AdjustRefIndexInternal(cursor, decls, index, asr, [](const List<IndexedDeclOrArg>& decls, vint index) -> const IndexToken&
	{
		return decls[index].token;
	});
}

void AdjustRefIndex(
	Ptr<CppTokenCursor>& cursor,
	const IndexMap& decls,
	IndexTracking& index,
	const AdjustSkippingResult& asr
)
{
	AdjustRefIndexInternal(cursor, decls, index, asr, [](const IndexMap& decls, vint index) -> const IndexToken&
	{
		return decls.Keys()[index];
	});
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
		holder = Ptr(new StreamHolder);
	}
	return holder->streamWriter;
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
	auto global = Ptr(new GlobalLinesRecord);
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
				vint lineNumber = wtoi(WString::CopyFrom(cursor->token.reading, cursor->token.length));
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
					auto filePathText = WString::CopyFrom(&buffer[0], (vint)(writing - &buffer[0]));
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
						auto flr = Ptr(new FileLinesRecord);
						flr->filePath = currentFilePath;
						global->fileLines.Add(currentFilePath, flr);
					}
				}
				SkipToken(cursor);
				continue;
			}
		GIVE_UP:
			cursor = oldCursor;
		}
	}

	return global;
}