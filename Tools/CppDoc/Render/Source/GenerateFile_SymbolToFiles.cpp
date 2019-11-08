#include "Render.h"

/***********************************************************************
GenerateSymbolToFiles
***********************************************************************/

void GenerateSymbolToFiles(Ptr<GlobalLinesRecord> global, Ptr<FileLinesRecord> flr, StreamWriter& writer)
{
	writer.WriteLine(L"symbolToFiles = {");
	bool firstFileMapping = false;
	auto writeFileMapping = [&](const WString& prefix, Ptr<Declaration> decl)
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
		writer.WriteString(prefix);
		writer.WriteString(decl->symbol->uniqueId);
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
	for (vint i = 0; i < flr->refSymbols.Count(); i++)
	{
		auto symbol = flr->refSymbols[i];
		switch (symbol->GetCategory())
		{
		case symbol_component::SymbolCategory::Normal:
			{
				if (auto decl = symbol->GetImplDecl_NFb())
				{
					writeFileMapping(L"NI$", decl);
				}
				for (vint j = 0; j < symbol->GetForwardDecls_N().Count(); j++)
				{
					auto decl = symbol->GetForwardDecls_N()[j];
					writeFileMapping(L"NF[" + itow(j) + L"]$", decl);
				}
			}
			break;
		case symbol_component::SymbolCategory::Function:
			{
				auto& symbols = symbol->GetImplSymbols_F();
				for (vint j = 0; j < symbols.Count(); j++)
				{
					writeFileMapping(L"FB$", symbols[j]->GetImplDecl_NFb());
				}
			}
			{
				auto& symbols = symbol->GetForwardSymbols_F();
				for (vint j = 0; j < symbols.Count(); j++)
				{
					writeFileMapping(L"FB$", symbols[j]->GetForwardDecl_Fb());
				}
			}
			break;
		case symbol_component::SymbolCategory::FunctionBody:
			throw UnexpectedSymbolCategoryException();
		}
	}
}