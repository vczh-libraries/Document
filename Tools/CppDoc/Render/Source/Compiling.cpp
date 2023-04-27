#include "Render.h"

/***********************************************************************
Compile
  Compile Input.cpp and index all useful tokens
***********************************************************************/

void Compile(
	Ptr<RegexLexer> lexer,
	FilePath pathInput,						// (in)  Input.cpp
	IndexResult& result,					// (out) indexing
	IProgressReporter* progressReporter
)
{
	// program input and index tokens
	result.input = File(pathInput).ReadAllTextByBom();
	CppTokenReader reader(lexer, result.input);
	auto cursor = reader.GetFirstToken();

	result.pa = {
		Ptr(new RootSymbol),
		ITsysAlloc::Create(),
		new IndexRecorder(result, progressReporter, result.input.Length())
	};

	result.program = ParseProgram(result.pa, cursor);
	EvaluateProgram(result.pa, result.program);
}

void GenerateUniqueId(
	IndexResult& result,					// (out) indexing
	IProgressReporter* progressReporter
)
{
	// collect all declaration names
	for (vint i = 0; i < result.ids.Count(); i++)
	{
		if (progressReporter)
		{
			progressReporter->OnProgress((vint)IProgressReporter::ExtraPhases::UniqueId, i, result.ids.Count());
		}

		auto symbol = result.ids.Values()[i];
		switch (symbol->GetCategory())
		{
		case symbol_component::SymbolCategory::Normal:
			switch (symbol->kind)
			{
			case symbol_component::SymbolKind::GenericTypeArgument:
			case symbol_component::SymbolKind::GenericValueArgument:
				{
					auto tsysArg = symbol_type_resolving::GetTemplateArgumentKey(symbol);
					auto ga = tsysArg->GetGenericArg();
					auto argument = ga.spec->arguments[ga.argIndex];

					auto& name = argument.name;
					if (name.tokenCount > 0)
					{
						result.decls.Add({ IndexToken::GetToken(name), nullptr,symbol });
					}
				}
				break;
			default:
				if (auto decl = symbol->GetImplDecl_NFb())
				{
					auto& name = decl->name;
					if (name.tokenCount > 0)
					{
						result.decls.Add({ IndexToken::GetToken(name), decl,nullptr });
					}
				}
				for (vint i = 0; i < symbol->GetForwardDecls_N().Count(); i++)
				{
					auto decl = symbol->GetForwardDecls_N()[i];
					auto& name = decl->name;
					if (name.tokenCount > 0)
					{
						result.decls.Add({ IndexToken::GetToken(name), decl,nullptr });
					}
				}
			}
			break;
		case symbol_component::SymbolCategory::Function:
			break;
		case symbol_component::SymbolCategory::FunctionBody:
			if (auto decl = symbol->GetImplDecl_NFb())
			{
				auto& name = decl->name;
				if (name.tokenCount > 0)
				{
					result.decls.Add({ IndexToken::GetToken(name), decl,nullptr });
				}
			}
			if (auto decl = symbol->GetForwardDecl_Fb())
			{
				auto& name = decl->name;
				if (name.tokenCount > 0)
				{
					result.decls.Add({ IndexToken::GetToken(name), decl,nullptr });
				}
			}
			break;
		}
	}
	
	if (result.decls.Count() > 0)
	{
		Sort<IndexedDeclOrArg>(&result.decls[0], result.decls.Count(), [](const IndexedDeclOrArg& da1, const IndexedDeclOrArg& da2)
		{
			return IndexToken::Compare(da1.token, da2.token);
		});
	}
}