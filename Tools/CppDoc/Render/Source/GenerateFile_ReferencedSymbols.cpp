#include "Render.h"

/***********************************************************************
GenerateReferencedSymbols
***********************************************************************/

void GenerateReferencedSymbols(Ptr<FileLinesRecord> flr, StreamWriter& writer)
{
	Dictionary<WString, Symbol*> referencedSymbols;
	for (vint i = 0; i < flr->refSymbols.Count(); i++)
	{
		auto symbol = flr->refSymbols[i];
		referencedSymbols.Add(GetSymbolId(symbol), symbol);
	}

	writer.WriteLine(L"referencedSymbols = {");
	for (vint i = 0; i < referencedSymbols.Count(); i++)
	{
		auto symbol = referencedSymbols.Values()[i];
		writer.WriteString(L"    \'");
		writer.WriteString(referencedSymbols.Keys()[i]);
		writer.WriteLine(L"\': {");

		writer.WriteString(L"        \'displayNameInHtml\': \'");
		writer.WriteString(GetSymbolDisplayNameInHtml(symbol));
		writer.WriteLine(L"\',");

		List<WString> impls, decls;
		EnumerateDecls(symbol, [&](DeclOrArg declOrArg, bool isImpl, vint index)
		{
			if (isImpl)
			{
				impls.Add(GetDeclId(declOrArg));
			}
			else
			{
				decls.Add(GetDeclId(declOrArg));
			}
		});

		if (impls.Count() == 0)
		{
			writer.WriteLine(L"        \'impls\': [],");
		}
		else
		{
			writer.WriteLine(L"        \'impls\': [");
			for (vint j = 0; j < impls.Count(); j++)
			{
				writer.WriteString(L"            \'");
				writer.WriteString(impls[j]);
				if (j == impls.Count() - 1)
				{
					writer.WriteLine(L"\'");
				}
				else
				{
					writer.WriteLine(L"\',");
				}
			}
			writer.WriteLine(L"        ],");
		}

		if (decls.Count() == 0)
		{
			writer.WriteLine(L"        \'decls\': []");
		}
		else
		{
			writer.WriteLine(L"        \'decls\': [");
			for (vint j = 0; j < decls.Count(); j++)
			{
				writer.WriteString(L"            \'");
				writer.WriteString(decls[j]);
				if (j == decls.Count() - 1)
				{
					writer.WriteLine(L"\'");
				}
				else
				{
					writer.WriteLine(L"\',");
				}
			}
			writer.WriteLine(L"        ]");
		}

		if (i == flr->refSymbols.Count() - 1)
		{
			writer.WriteLine(L"    }");
		}
		else
		{
			writer.WriteLine(L"    },");
		}
	}
	writer.WriteLine(L"};");
}