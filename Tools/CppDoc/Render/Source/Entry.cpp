#include "Render.h"

/***********************************************************************
ProgressReporter
***********************************************************************/

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

/***********************************************************************
IndexCppCode
***********************************************************************/

void IndexCppCode(
	// input
	FileGroupConfig&		fileGroups,					// source folder (ending with FilePath::Delimiter) -> category name
	File					preprocessedFile,			// .I file generated by cl.exe
	Ptr<RegexLexer>			lexer,						// C++ lexical analyzer

	// output
	FilePath				pathPreprocessed,			// cache: preprocessed file
	FilePath				pathInput,					// cache: compacted preprocessed file, removing all empty, space or # lines
	FilePath				pathMapping,				// cache: line mapping between pathPreprocessed and pathInput

	Folder					folderOutput,				// root output folder
	Folder					folderSource,				// folder containing generated HTML files
	Folder					folderFragment,				// folder containing symbol index fragments
	Folder					folderReference				// folder containing reference files
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
		GenerateFile(global, flr, indexResult, folderOutput.GetFilePath() / L"SourceFiles" / (flr->htmlFileName + L".html"));
	}

	SortedList<WString> predefinedFileGroups;
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
		CopyFrom(
			predefinedFileGroups,
			From(fileGroups)
				.Select([](Tuple<WString, WString> fileGroup)
				{
					return fileGroup.f1;
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
				if (INVLOC.StartsWith(sdkPath.GetFullPath() + WString::FromChar(FilePath::Delimiter), candidate.GetFullPath() + WString::FromChar(FilePath::Delimiter), Locale::Normalization::IgnoreCase))
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
			fileGroups.Add({ sdkPath.GetFullPath() + WString::FromChar(FilePath::Delimiter), L"In SDK: " + sdkPath.GetFullPath() });
		}
	}

	Console::WriteLine(L"        FileIndex.html");
	GenerateFileIndex(
		global,
		folderOutput.GetFilePath() / L"FileIndex.html",
		fileGroups
	);

	Console::WriteLine(L"        SymbolIndex.html");
	auto rootGroup = GenerateSymbolIndex(
		global,
		indexResult,
		folderOutput.GetFilePath() / L"SymbolIndex.html",
		folderFragment.GetFilePath(),
		fileGroups,
		&progressReporter
	);
	progressReporter.FinishPhase();

	Console::WriteLine(L"        ReferenceIndex.xml");
	GenerateReferenceIndex(
		global,
		indexResult,
		rootGroup,
		folderOutput.GetFilePath() / L"ReferenceIndex.xml",
		folderReference.GetFilePath(),
		fileGroups,
		predefinedFileGroups,
		&progressReporter
	);
	progressReporter.FinishPhase();

	Console::WriteLine(L"    Finished");
}