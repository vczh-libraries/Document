#include "Ast_Resolving_PSO.h"

namespace partial_specification_ordering
{
	/***********************************************************************
	AssignPSPrimary
	***********************************************************************/

	void AssignPSPrimary(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, Symbol* symbol)
	{
		switch (symbol->kind)
		{
		case CLASS_SYMBOL_KIND:
		case symbol_component::SymbolKind::ValueAlias:
			if (!symbol->GetPSPrimary_NF())
			{
				auto decl = symbol->GetAnyForwardDecl<Declaration>();
				auto candidates = symbol->GetParentScope()->TryGetChildren_NFb(decl->name.name);
				if (!candidates) throw StopParsingException(cursor);
				for (vint i = 0; i < candidates->Count(); i++)
				{
					auto candidate = candidates->Get(i).Obj();
					if (candidate->kind != symbol_component::SymbolKind::FunctionSymbol)
					{
						symbol->AssignPSPrimary_NF(candidate);
					}
				}
			}
			break;
		case symbol_component::SymbolKind::FunctionSymbol:
			if (!symbol->GetPSPrimary_NF())
			{
			}
			break;
		default:
			throw StopParsingException(cursor);
		}
	}
}