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
	WriteMappingFile(pathMapping, mapping);
}