#include "Render.h"

/***********************************************************************
Compile
***********************************************************************/

void Compile(Ptr<RegexLexer> lexer, FilePath pathInput, IndexResult& result)
{
	WString input = File(pathInput).ReadAllTextByBom();
	CppTokenReader reader(lexer, input);
	auto cursor = reader.GetFirstToken();

	result.pa = { new Symbol(symbol_component::SymbolCategory::Normal), ITsysAlloc::Create(), new IndexRecorder(result) };
	auto program = ParseProgram(result.pa, cursor);
	EvaluateProgram(result.pa, program);

	result.pa.root->GenerateUniqueId(result.ids, L"");
	for (vint i = 0; i < result.ids.Count(); i++)
	{
		auto symbol = result.ids.Values()[i];
		switch (symbol->GetCategory())
		{
		case symbol_component::SymbolCategory::Normal:
			if (auto decl = symbol->GetImplDecl_NFb())
			{
				auto& name = decl->name;
				if (name.tokenCount > 0)
				{
					result.decls.Add(IndexToken::GetToken(name), decl);
				}
			}
			for (vint i = 0; i < symbol->GetForwardDecls_N().Count(); i++)
			{
				auto decl = symbol->GetForwardDecls_N()[i];
				auto& name = decl->name;
				if (name.tokenCount > 0)
				{
					result.decls.Add(IndexToken::GetToken(name), decl);
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
					result.decls.Add(IndexToken::GetToken(name), decl);
				}
			}
			if (auto decl = symbol->GetForwardDecl_Fb())
			{
				auto& name = decl->name;
				if (name.tokenCount > 0)
				{
					result.decls.Add(IndexToken::GetToken(name), decl);
				}
			}
			break;
		}
	}
}