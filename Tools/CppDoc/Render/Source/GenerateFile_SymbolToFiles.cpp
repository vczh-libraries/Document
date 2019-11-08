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
		switch (symbol->GetCategory())
		{
		case symbol_component::SymbolCategory::Normal:
			{
				if (auto decl = symbol->GetImplDecl_NFb())
				{
					symbolToFiles.Add(L"NI$" + symbol->uniqueId, decl);
				}
				for (vint j = 0; j < symbol->GetForwardDecls_N().Count(); j++)
				{
					auto decl = symbol->GetForwardDecls_N()[j];
					symbolToFiles.Add(L"NF[" + itow(j) + L"]$" + symbol->uniqueId, decl);
				}
			}
			break;
		case symbol_component::SymbolCategory::Function:
			{
				auto& symbols = symbol->GetImplSymbols_F();
				for (vint j = 0; j < symbols.Count(); j++)
				{
					auto symbol = symbols[j];
					symbolToFiles.Add(L"FB$" + symbol->uniqueId, symbol->GetImplDecl_NFb());
				}
			}
			{
				auto& symbols = symbol->GetForwardSymbols_F();
				for (vint j = 0; j < symbols.Count(); j++)
				{
					auto symbol = symbols[j];
					symbolToFiles.Add(L"FB$" + symbol->uniqueId, symbol->GetForwardDecl_Fb());
				}
			}
			break;
		case symbol_component::SymbolCategory::FunctionBody:
			throw UnexpectedSymbolCategoryException();
		}
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