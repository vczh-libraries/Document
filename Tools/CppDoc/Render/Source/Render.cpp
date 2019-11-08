#include "Render.h"

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