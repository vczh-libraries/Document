#include "Render.h"

/***********************************************************************
WriteFileMapping
***********************************************************************/

void WriteFileMapping(Ptr<GlobalLinesRecord> global, Ptr<FileLinesRecord> flr, bool& firstFileMapping, const WString& declId, Ptr<Declaration> decl, StreamWriter& writer)
{
	vint index = global->declToFiles.Keys().IndexOf(decl.Obj());
	if (index == -1) return;

	if (!firstFileMapping)
	{
		firstFileMapping = true;
	}
	else
	{
		writer.WriteLine(L",");
	}

	writer.WriteString(L"    \'");
	writer.WriteString(declId);
	writer.WriteString(L"\': ");

	auto filePath = global->declToFiles.Values()[index];
	auto flrTarget = global->fileLines[filePath];
	if (flrTarget == flr)
	{
		writer.WriteString(L"null");
	}
	else
	{
		writer.WriteString(L"{ \'htmlFileName\': \'");
		writer.WriteString(flrTarget->htmlFileName);
		writer.WriteString(L"\', \'displayName\': \'");
		writer.WriteString(flrTarget->filePath.GetName());
		writer.WriteString(L"\' }");
	}
};

/***********************************************************************
GenerateSymbolToFiles
***********************************************************************/

void GenerateSymbolToFiles(Ptr<GlobalLinesRecord> global, Ptr<FileLinesRecord> flr, StreamWriter& writer)
{
	Dictionary<WString, Ptr<Declaration>> symbolToFiles;

	for (vint i = 0; i < flr->refSymbols.Count(); i++)
	{
		auto symbol = flr->refSymbols[i];
		EnumerateDecls(symbol, [&](Ptr<Declaration> decl, bool isImpl, vint index)
		{
			symbolToFiles.Add(GetDeclId(decl), decl);
		});
	}

	bool firstFileMapping = false;
	writer.WriteLine(L"symbolToFiles = {");
	for (vint i = 0; i < symbolToFiles.Count(); i++)
	{
		WriteFileMapping(global, flr, firstFileMapping, symbolToFiles.Keys()[i], symbolToFiles.Values()[i], writer);
	}
	writer.WriteLine(L"");
	writer.WriteLine(L"};");
}