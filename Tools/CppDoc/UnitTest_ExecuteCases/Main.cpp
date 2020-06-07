#include <Render.h>

class ProgressReporter : public Object, public virtual IProgressReporter
{
private:
	static const vint		MaxProgress = 2000;
	static const vint		MaxProgressPerLine = 100;

	vint					currentPhase = 0;
	vint					currentProgress = 0;

	void UpdateProgressInCurrentPhase(vint progress)
	{
		if (progress > currentProgress)
		{
			for (vint i = currentProgress + 1; i <= progress; i++)
			{
				if (i % MaxProgressPerLine == 1)
				{
					vint line = i / MaxProgressPerLine;
					Console::Write(L"     " + itow(currentPhase) + L"[" + itow((line / 10) % 10) + itow(line % 10) + L"]: ");
				}
				Console::Write(L"*");
				if (i % MaxProgressPerLine == 0)
				{
					Console::WriteLine(L"");
				}
			}
			currentProgress = progress;
		}
	}

public:
	void FinishPhase()
	{
		UpdateProgressInCurrentPhase(MaxProgress);
		currentPhase++;
		currentProgress = 0;
	}

	void OnProgress(vint phase, vint position, vint length)
	{
		if (phase != currentPhase)
		{
			for (vint i = currentPhase; i < phase; i++)
			{
				FinishPhase();
			}
		}

		double ratio = (double)position / length;
		if (ratio < 0) ratio = 0;
		if (ratio > 1) ratio = 1;
		UpdateProgressInCurrentPhase((vint)(ratio*MaxProgress));
	}
};

void IndexCppCode(
	// input
	FileGroupConfig&		fileGroups,					// source folder (ending with FilePath::Delimiter) -> category name
	File					preprocessedFile,			// .I file generated by cl.exe
	Ptr<RegexLexer>			lexer,						// C++ lexical analyzer

	// output
	FilePath				pathPreprocessed,			// cache: preprocessed file
	FilePath				pathInput,					// cache: compacted preprocessed file, removing all empty, space or # lines
	FilePath				pathMapping,				// cache: line mapping between pathPreprocessed and pathInput
	Folder					folderOutput				// folder containing generated HTML files
)
{
	Console::WriteLine(preprocessedFile.GetFilePath().GetFullPath());
	Console::WriteLine(L"    Preprocessing");
	PreprocessedFileToCompactCodeAndMapping(
		lexer,
		preprocessedFile.GetFilePath(),
		pathPreprocessed,
		pathInput,
		pathMapping
	);

	ProgressReporter progressReporter;
	Console::WriteLine(L"    Compiling");
	IndexResult indexResult;
	Compile(
		lexer,
		pathInput,
		indexResult,
		&progressReporter
	);
	progressReporter.FinishPhase();

	Console::WriteLine(L"    Generating UniqueIds");
	indexResult.pa.root->GenerateUniqueId(indexResult.ids, L"");

	Console::WriteLine(L"    Indexing Symbols");
	GenerateUniqueId(
		indexResult,
		&progressReporter
	);
	progressReporter.FinishPhase();

	Console::WriteLine(L"    Generating HTML");
	auto global = Collect(
		lexer,
		pathPreprocessed,
		pathInput,
		pathMapping,
		indexResult,
		&progressReporter
	);
	progressReporter.FinishPhase();

	Console::WriteLine(L"    Writing Files");
	for (vint i = 0; i < global->fileLines.Keys().Count(); i++)
	{
		auto flr = global->fileLines.Values()[i];
		Console::WriteLine(L"        " + flr->htmlFileName + L".html");
		GenerateFile(global, flr, indexResult, folderOutput.GetFilePath() / (flr->htmlFileName + L".html"));
	}

	{
		// collect all folders of predefined file groups
		List<WString> sourcePrefixes;
		CopyFrom(
			sourcePrefixes,
			From(fileGroups)
				.Select([](Tuple<WString, WString> fileGroup)
				{
					return fileGroup.f0;
				})
			);

		// collect all top level folders of source files
		SortedList<FilePath> sdkPaths;
		for (vint i = 0; i < global->fileLines.Count(); i++)
		{
			auto filePath = global->fileLines.Values()[i]->filePath;
			// if this file is not located in any predefined file group
			if (!From(sourcePrefixes)
				.Any([=](const WString& prefix)
				{
					return INVLOC.StartsWith(filePath.GetFullPath(), prefix, Locale::Normalization::IgnoreCase);
				}))
			{
				// and the folder
				auto sdkPath = filePath.GetFolder();
				if (!sdkPaths.Contains(sdkPath))
				{
					sdkPaths.Add(sdkPath);
				}
			}
		}

		// remove any non-top level folders
		for (vint i = sdkPaths.Count() - 1; i >= 0; i--)
		{
			auto sdkPath = sdkPaths[i];
			for (vint j = i - 1; i >= 0; i--)
			{
				auto candidate = sdkPaths[j];
				if (INVLOC.StartsWith(sdkPath.GetFullPath() + FilePath::Delimiter, candidate.GetFullPath() + FilePath::Delimiter, Locale::Normalization::IgnoreCase))
				{
					sdkPaths.RemoveAt(i);
					break;
				}
			}
		}

		// generate file groups for remaining files
		for (vint i = 0; i < sdkPaths.Count(); i++)
		{
			auto sdkPath = sdkPaths[i];
			fileGroups.Add({ sdkPath.GetFullPath() + FilePath::Delimiter, L"In SDK: " + sdkPath.GetFullPath() });
		}
	}

	Console::WriteLine(L"        FileIndex.html");
	GenerateFileIndex(
		global,
		folderOutput.GetFilePath() / L"FileIndex.html",
		fileGroups
	);

	Console::WriteLine(L"        SymbolIndex.html");
	GenerateSymbolIndex(
		global,
		indexResult,
		folderOutput.GetFilePath() / L"SymbolIndex.html",
		fileGroups,
		&progressReporter
	);
	progressReporter.FinishPhase();

	Console::WriteLine(L"    Finished");
}

/***********************************************************************
Main

Set root folder to /Tools/Demos
Open http://127.0.0.1:8080/_Test/FileIndex.html
Open http://127.0.0.1:8080/Calculator/FileIndex.html
Open http://127.0.0.1:8080/TypePrinter/FileIndex.html
Open http://127.0.0.1:8080/STL/FileIndex.html
Open http://127.0.0.1:8080/Gaclib/FileIndex.html
***********************************************************************/

int main()
{
	List<File> preprocessedFiles;
	preprocessedFiles.Add(File(L"../UnitTest_Cases/_Test.i"));
	//preprocessedFiles.Add(File(L"../UnitTest_Cases/Calculator.i"));
	//preprocessedFiles.Add(File(L"../UnitTest_Cases/TypePrinter.i"));
	//preprocessedFiles.Add(File(L"../UnitTest_Cases/STL.i"));
	//preprocessedFiles.Add(File(L"../UnitTest_Cases/Gaclib.i"));
	
	Console::WriteLine(L"Cleaning ...");
	FOREACH(File, file, preprocessedFiles)
	{
		WString projectName = file.GetFilePath().GetName();
		projectName = projectName.Left(projectName.Length() - 1);
		Folder folderInput(file.GetFilePath().GetFullPath() + L".Input");
		Folder folderOutput(file.GetFilePath().GetFolder() / (L"../../Demos/" + projectName));

		if (folderInput.Exists())
		{
			folderInput.Delete(true);
		}
		if (folderOutput.Exists())
		{
			folderOutput.Delete(true);
		}
	}
	auto lexer = CreateCppLexer();

	FOREACH(File, file, preprocessedFiles)
	{
		WString projectName = file.GetFilePath().GetName();
		projectName = projectName.Left(projectName.Length() - 1);
		Folder folderInput(file.GetFilePath().GetFullPath() + L".Input");
		Folder folderOutput(file.GetFilePath().GetFolder() / (L"../../Demos/" + projectName));

		auto pathPreprocessed = folderInput.GetFilePath() / L"Preprocessed.cpp";
		auto pathInput = folderInput.GetFilePath() / L"Input.cpp";
		auto pathMapping = folderInput.GetFilePath() / L"Mapping.bin";

		if (!folderInput.Exists())
		{
			folderInput.Create(true);
		}
		if (!folderOutput.Exists())
		{
			folderOutput.Create(true);
		}

		FileGroupConfig fileGroups;
		fileGroups.Add({ file.GetFilePath().GetFolder().GetFullPath() + FilePath::Delimiter, L"Source Code of this Project" });
		fileGroups.Add({ FilePath(L"../../../../Vlpp").GetFullPath() + FilePath::Delimiter, L"Vlpp" });
		fileGroups.Add({ FilePath(L"../../../../VlppOS").GetFullPath() + FilePath::Delimiter, L"VlppOS" });
		fileGroups.Add({ FilePath(L"../../../../VlppRegex").GetFullPath() + FilePath::Delimiter, L"VlppRegex" });
		fileGroups.Add({ FilePath(L"../../../../VlppReflection").GetFullPath() + FilePath::Delimiter, L"VlppReflection" });
		fileGroups.Add({ FilePath(L"../../../../VlppParser").GetFullPath() + FilePath::Delimiter, L"VlppParser" });
		fileGroups.Add({ FilePath(L"../../../../Workflow").GetFullPath() + FilePath::Delimiter, L"Workflow" });
		fileGroups.Add({ FilePath(L"../../../../GacUI").GetFullPath() + FilePath::Delimiter, L"GacUI" });
		IndexCppCode(fileGroups, file, lexer, pathPreprocessed, pathInput, pathMapping, folderOutput);
	}

	return 0;
}