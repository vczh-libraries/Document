#include <Render.h>
#include <ExportReflectableTypes.h>

/***********************************************************************
Main

Set root folder to /Tools/Demos
Open http://127.0.0.1:8080/Gaclib/FileIndex.html
***********************************************************************/

int main()
{
	File file(L"../UnitTest_Cases/Gaclib.i");

	Console::WriteLine(L"Cleaning ...");
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
	{
		WString projectName = file.GetFilePath().GetName();
		projectName = projectName.Left(projectName.Length() - 1);
		Folder folderInput(file.GetFilePath().GetFullPath() + L".Input");
		Folder folderOutput(file.GetFilePath().GetFolder() / (L"../../Demos/" + projectName));
		Folder folderReference(folderOutput.GetFilePath() / L"References");

		auto pathPreprocessed = folderInput.GetFilePath() / L"Preprocessed.cpp";
		auto pathInput = folderInput.GetFilePath() / L"Input.cpp";
		auto pathMapping = folderInput.GetFilePath() / L"Mapping.bin";

		folderInput.Create(true);
		folderOutput.Create(true);
		folderReference.Create(true);

		FileGroupConfig fileGroups;
		fileGroups.Add({ file.GetFilePath().GetFolder().GetFullPath() + WString::FromChar(FilePath::Delimiter), L"Source Code of this Project" });
		fileGroups.Add({ FilePath(L"../../../../Vlpp").GetFullPath() + WString::FromChar(FilePath::Delimiter), L"Vlpp" });
		fileGroups.Add({ FilePath(L"../../../../VlppOS").GetFullPath() + WString::FromChar(FilePath::Delimiter), L"VlppOS" });
		fileGroups.Add({ FilePath(L"../../../../VlppRegex").GetFullPath() + WString::FromChar(FilePath::Delimiter), L"VlppRegex" });
		fileGroups.Add({ FilePath(L"../../../../VlppReflection").GetFullPath() + WString::FromChar(FilePath::Delimiter), L"VlppReflection" });
		fileGroups.Add({ FilePath(L"../../../../VlppParser").GetFullPath() + WString::FromChar(FilePath::Delimiter), L"VlppParser" });
		fileGroups.Add({ FilePath(L"../../../../VlppParser2").GetFullPath() + WString::FromChar(FilePath::Delimiter), L"VlppParser2" });
		fileGroups.Add({ FilePath(L"../../../../Workflow").GetFullPath() + WString::FromChar(FilePath::Delimiter), L"Workflow" });
		fileGroups.Add({ FilePath(L"../../../../GacUI").GetFullPath() + WString::FromChar(FilePath::Delimiter), L"GacUI" });
		IndexCppCode(
			fileGroups,
			file,
			lexer,
			pathPreprocessed,
			pathInput,
			pathMapping,
			folderOutput,
			folderReference
		);

		ExportReflectableTypes(folderOutput);
	}

	return 0;
}