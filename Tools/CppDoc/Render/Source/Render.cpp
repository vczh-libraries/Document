#include "Render.h"

/***********************************************************************
EnumerateDecls
***********************************************************************/

void EnumerateDecls(Symbol* symbol, const Func<void(Ptr<Declaration>, bool, vint)>& callback)
{
	switch (symbol->GetCategory())
	{
	case symbol_component::SymbolCategory::Normal:
		{
			if (auto decl = symbol->GetImplDecl_NFb())
			{
				callback(decl, true, -1);
			}
			for (vint j = 0; j < symbol->GetForwardDecls_N().Count(); j++)
			{
				auto decl = symbol->GetForwardDecls_N()[j];
				callback(decl, false, j);
			}
		}
		break;
	case symbol_component::SymbolCategory::Function:
		{
			auto& symbols = symbol->GetImplSymbols_F();
			for (vint j = 0; j < symbols.Count(); j++)
			{
				auto functionBodySymbol = symbols[j].Obj();
				callback(functionBodySymbol->GetImplDecl_NFb(), true, j);
			}
		}
		{
			auto& symbols = symbol->GetForwardSymbols_F();
			for (vint j = 0; j < symbols.Count(); j++)
			{
				auto functionBodySymbol = symbols[j].Obj();
				callback(functionBodySymbol->GetForwardDecl_Fb(), false, j);
			}
		}
		break;
	case symbol_component::SymbolCategory::FunctionBody:
		throw UnexpectedSymbolCategoryException();
	}
}

/***********************************************************************
GetDeclId
***********************************************************************/

WString GetDeclId(Ptr<Declaration> decl)
{
	switch (decl->symbol->GetCategory())
	{
	case symbol_component::SymbolCategory::Normal:
		{
			if (decl == decl->symbol->GetImplDecl_NFb())
			{
				return L"NI$" + decl->symbol->uniqueId;
			}
			else
			{
				vint index = decl->symbol->GetForwardDecls_N().IndexOf(decl.Obj());
				if(index==-1) throw L"Unrecognizable declaration.";
				return L"NF[" + itow(index) + L"]$" + decl->symbol->uniqueId;
			}
		}
		break;
	case symbol_component::SymbolCategory::FunctionBody:
		{
			return L"FB$" + decl->symbol->uniqueId;
		}
		break;
	}
	throw L"Unrecognizable declaration.";
}

/***********************************************************************
GetSymbolDivClass
***********************************************************************/

const wchar_t* GetSymbolDivClass(Symbol* symbol)
{
	switch (symbol->kind)
	{
	case symbol_component::SymbolKind::Enum:
	case symbol_component::SymbolKind::Class:
	case symbol_component::SymbolKind::Struct:
	case symbol_component::SymbolKind::Union:
	case symbol_component::SymbolKind::TypeAlias:
	case symbol_component::SymbolKind::GenericTypeArgument:
		return L"cpp_type";
	case symbol_component::SymbolKind::EnumItem:
		return L"cpp_enum";
	case symbol_component::SymbolKind::Variable:
		if (auto parent = symbol->GetParentScope())
		{
			if (parent->GetImplDecl_NFb<FunctionDeclaration>())
			{
				return L"cpp_argument";
			}
			else if (parent->GetImplDecl_NFb<ClassDeclaration>())
			{
				return L"cpp_field";
			}
		}
		break;
	case symbol_component::SymbolKind::FunctionBodySymbol:
		return L"cpp_function";
	}
	return nullptr;
}