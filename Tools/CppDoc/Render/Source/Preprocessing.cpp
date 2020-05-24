#include "Render.h"

/***********************************************************************
ReadMappingFile
***********************************************************************/

void ReadMappingFile(FilePath pathMapping, Array<TokenSkipping>& mapping)
{
	FileStream fileStream(pathMapping.GetFullPath(), FileStream::ReadOnly);
	vint count;
	fileStream.Read(&count, sizeof(count));
	mapping.Resize(count);

	if (count > 0)
	{
		fileStream.Read(&mapping[0], sizeof(TokenSkipping)*count);
	}
}

/***********************************************************************
WriteMappingFile
***********************************************************************/

void WriteMappingFile(FilePath pathMapping, List<TokenSkipping>& mapping)
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

/***********************************************************************
PreprocessedFileToCompactCodeAndMapping
  Make the code compact and remove all
	#pragma
    comments
	empty lines
	leading spaces
	trailing spaces
***********************************************************************/

void PreprocessedFileToCompactCodeAndMapping(
	Ptr<RegexLexer> lexer,
	FilePath pathInput,			// (in)  .I
	FilePath pathPreprocessed,	// (out) Preprocessed.cpp
	FilePath pathOutput,		// (out) Input.cpp
	FilePath pathMapping		// (out) Mapping.bin
)
{
	// copy from .I to Preprocessed.cpp
	WString input = File(pathInput).ReadAllTextByBom();
	File(pathPreprocessed).WriteAllText(input, true, BomEncoder::Utf16);

	// extract useful tokens from Preprocessed.cpp to Input.cpp
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
			// leading spaces, trailing spaces, comments, pragmas are skipped
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

					// search until a token that cannot be skipped
					// group all continuing tokens to skip in one single TokenSkipping
					while (cursor)
					{
						ts.rowUntil = cursor->token.rowStart;
						ts.columnUntil = cursor->token.columnStart;

						if ((CppTokens)cursor->token.token == CppTokens::SHARP)
						{
							// if we see a #pragma
							vint parenthesisCounter = 0;
							vint lastSharpRowEnd = ts.rowUntil;
							bool lastTokenIsSkipping = false;

							while (cursor)
							{
								ts.rowUntil = cursor->token.rowStart;
								ts.columnUntil = cursor->token.columnStart;

								// if this #pragma reach a line-break, stops without consuming the line-break
								if (lastTokenIsSkipping && lastSharpRowEnd != ts.rowUntil)
								{
									goto STOP_SHARP;
								}
								lastSharpRowEnd = ts.rowUntil;

								switch ((CppTokens)cursor->token.token)
								{
								case CppTokens::LPARENTHESIS:
									// if we see a (, count them
									lastTokenIsSkipping = false;
									parenthesisCounter++;
									break;
								case CppTokens::RPARENTHESIS:
									// if we see a ), count them
									// if this ) closes all (, stops
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
									// spaces and comments are allow in a #pragma, as long as they have no line-break
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
						// if all continuing spaces and comments contain no line-break, don't skip them
						// #pragma will always be skipped because it is not valid to write code in that line
						// so there will always be a line-break after it
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
				// if the token cannot be skipped, write to Input.cpp
				writer.WriteString(cursor->token.reading, cursor->token.length);
				SkipToken(cursor);
			}
		}
	}

	// record all skipped tokens in Mapping.bin
	WriteMappingFile(pathMapping, mapping);
}