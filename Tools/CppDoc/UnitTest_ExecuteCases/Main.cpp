#include <Parser.h>

using namespace vl::stream;
using namespace vl::filesystem;
using namespace vl::console;

void CleanUpPreprocessFile(FilePath input, FilePath preprocessed, FilePath output, FilePath mapping)
{
	File(preprocessed).WriteAllText(File(input).ReadAllTextByBom(), false, BomEncoder::Utf8);
}

int main()
{
	Folder folderCase(L"../UnitTest_Cases");
	List<File> files;
	folderCase.GetFiles(files);
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
				file.GetFilePath(),
				folderOutput.GetFilePath() / L"Preprocessed.txt",
				folderOutput.GetFilePath() / L"Input.txt",
				folderOutput.GetFilePath() / L"Mapping.txt"
			);
		}
	}
	return 0;
}