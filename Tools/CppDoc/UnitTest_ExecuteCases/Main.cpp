#include <Render.h>

/***********************************************************************
Main

Set root folder which contains UnitTest_Cases.vcxproj
Open http://127.0.0.1:8080/Calculator.i.Output/FileIndex.html
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
			PreprocessedFileToCompactCodeAndMapping(
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
				GenerateFile(global, flr, indexResult, folderOutput.GetFilePath() / (flr->htmlFileName + L".html"));
			}

			FileGroupConfig fileGroups;
			{
				auto projectFolder = wupper(folderOutput.GetFilePath().GetFolder().GetFullPath() + FilePath::Delimiter);
				fileGroups.Add({ projectFolder, L"In This Project" });

				SortedList<FilePath> sdkPaths;
				for (vint i = 0; i < global->fileLines.Count(); i++)
				{
					auto filePath = global->fileLines.Values()[i]->filePath;
					if (!INVLOC.StartsWith(wupper(filePath.GetFullPath()), projectFolder, Locale::Normalization::None))
					{
						auto sdkPath = filePath.GetFolder();
						if (!sdkPaths.Contains(sdkPath))
						{
							sdkPaths.Add(sdkPath);
							fileGroups.Add({ wupper(sdkPath.GetFullPath() + FilePath::Delimiter), L"In SDK: " + sdkPath.GetFullPath() });
						}
					}
				}
			}

			GenerateFileIndex(global, folderOutput.GetFilePath() / L"FileIndex.html", fileGroups);
			GenerateSymbolIndex(global, indexResult, folderOutput.GetFilePath() / L"SymbolIndex.html", fileGroups);
		}
	}

	//{
	//	Folder folderOutput(L"../../../.Output/Import/Preprocessed.txt.Output");
	//	auto pathPreprocessed = folderOutput.GetFilePath() / L"Preprocessed.cpp";
	//	auto pathInput = folderOutput.GetFilePath() / L"Input.cpp";
	//	auto pathMapping = folderOutput.GetFilePath() / L"Mapping.bin";

	//	if (!folderOutput.Exists())
	//	{
	//		folderOutput.Create(false);
	//	}

	//	Console::WriteLine(L"Preprocessing Preprocessed.txt ...");
	//	CleanUpPreprocessFile(
	//		lexer,
	//		folderOutput.GetFilePath() / L"../Preprocessed.txt",
	//		pathPreprocessed,
	//		pathInput,
	//		pathMapping
	//	);
	//}

	return 0;
}