#include <Parser.h>

using namespace vl::stream;
using namespace vl::filesystem;
using namespace vl::console;

// skip these tokens to in space
struct TokenSkipping
{
	vint		rowSkip = -1;
	vint		columnSkip = -1;
	vint		rowUntil = -1;
	vint		columnUntil = -1;
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
	File(pathPreprocessed).WriteAllText(input, false, BomEncoder::Utf8);

	List<TokenSkipping> mapping;
	{
		CppTokenReader reader(lexer, input, false);
		auto cursor = reader.GetFirstToken();

		FileStream fileStream(pathOutput.GetFullPath(), FileStream::WriteOnly);
		Utf8Encoder encoder;
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
						ts.rowUntil = cursor->token.rowEnd;
						ts.columnUntil = cursor->token.columnEnd;

						vint lastSharpRowEnd = cursor->token.rowEnd;
						if ((CppTokens)cursor->token.token == CppTokens::SHARP)
						{
							vint parenthesisCounter = 0;
							while (cursor)
							{
								ts.rowStart = cursor->token.rowStart;
								ts.columnStart = cursor->token.columnStart;
								switch ((CppTokens)cursor->token.token)
								{
								case CppTokens::LPARENTHESIS:
									parenthesisCounter++;
									break;
								case CppTokens::RPARENTHESIS:
									parenthesisCounter--;
									if (parenthesisCounter == 0) goto STOP_SHARP;
									break;
								case CppTokens::SPACE:
								case CppTokens::COMMENT1:
								case CppTokens::COMMENT2:
									if (cursor->token.rowEnd != lastSharpRowEnd)
									{
										goto STOP_SHARP;
									}
								default:
									lastSharpRowEnd = cursor->token.rowEnd;
								}
								cursor = cursor->Next();
							}
						}
					STOP_SHARP:

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
					STOP_SKIPPING:

					if (ts.rowSkip == ts.rowUntil)
					{
						cursor = oldCursor;
					}
					else
					{
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
			if (!folderOutput.Exists())
			{
				folderOutput.Create(false);
			}

			Console::WriteLine(L"Processing " + file.GetFilePath().GetName() + L" ...");
			CleanUpPreprocessFile(
				lexer,
				file.GetFilePath(),
				folderOutput.GetFilePath() / L"Preprocessed.txt",
				folderOutput.GetFilePath() / L"Input.txt",
				folderOutput.GetFilePath() / L"Mapping.txt"
			);
		}
	}
	return 0;
}