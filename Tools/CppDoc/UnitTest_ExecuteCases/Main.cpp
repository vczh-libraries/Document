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

void Compile(Ptr<RegexLexer> lexer, FilePath pathFolder, FilePath pathInput)
{
	WString input = File(pathInput).ReadAllTextByBom();
	CppTokenReader reader(lexer, input);
	auto cursor = reader.GetFirstToken();
	ParsingArguments pa(new Symbol, ITsysAlloc::Create(), nullptr);
	auto program = ParseProgram(pa, cursor);
	EvaluateProgram(pa, program);
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

			Console::WriteLine(L"Preprocessing " + file.GetFilePath().GetName() + L" ...");
			CleanUpPreprocessFile(
				lexer,
				file.GetFilePath(),
				folderOutput.GetFilePath() / L"Preprocessed.cpp",
				folderOutput.GetFilePath() / L"Input.cpp",
				folderOutput.GetFilePath() / L"Mapping.bin"
			);
		}
	}

	FOREACH(File, file, files)
	{
		if (wupper(file.GetFilePath().GetFullPath().Right(2)) == L".I")
		{
			Console::WriteLine(L"Compiling " + file.GetFilePath().GetName() + L" ...");
			Folder folderOutput(file.GetFilePath().GetFullPath() + L".Output");
			Compile(
				lexer,
				file.GetFilePath(),
				folderOutput.GetFilePath() / L"Input.cpp"
			);
		}
	}
	return 0;
}