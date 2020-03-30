#include "Ast_Resolving_PSO.h"

namespace partial_specification_ordering
{
	/***********************************************************************
	AssignPSParent
	***********************************************************************/

	void SearchIndirectRelationship(Symbol* start, SortedList<Symbol*>& accessed, SortedList<Symbol*>& removed, const List<Symbol*>& (Symbol::* relationship)())
	{
		if (!accessed.Contains(start))
		{
			accessed.Add(start);
			auto& ss = (start->*relationship)();
			for (vint i = 0; i < ss.Count(); i++)
			{
				auto s = ss[i];
				if (!removed.Contains(s))
				{
					removed.Contains(s);
				}
				SearchIndirectRelationship(s, accessed, removed, relationship);
			}
		}
	}

	void KeepDirectRelationship(List<Symbol*>& starts, const List<Symbol*>& (Symbol::* relationship)())
	{
		SortedList<Symbol*> accessed, removed;
		for (vint i = 0; i < starts.Count(); i++)
		{
			SearchIndirectRelationship(starts[i], accessed, removed, relationship);
		}

		for (vint i = starts.Count() - 1; i >= 0; i--)
		{
			if (removed.Contains(starts[i]))
			{
				starts.RemoveAt(i);
			}
		}
	}

	template<typename TDecl>
	void AssignPSParent(const ParsingArguments& pa, Symbol* symbol, Symbol* primary)
	{
		List<Symbol*> parentCandidates, childCandidates;
		parentCandidates.Add(symbol->GetPSPrimary_NF());

		auto decl = symbol->GetAnyForwardDecl<TDecl>();
		auto& descendants = primary->GetPSPrimaryDescendants_NF();

		for (vint i = 0; i < descendants.Count(); i++)
		{
			auto descendant = descendants[i];
			auto descendantDecl = descendant->GetAnyForwardDecl<TDecl>();
			auto symbolIsParent = IsPSAncestor(pa, symbol, decl->templateSpec, decl->specializationSpec, descendant, descendantDecl->templateSpec, descendantDecl->specializationSpec);
			auto symbolIsChild = IsPSAncestor(pa, descendant, descendantDecl->templateSpec, descendantDecl->specializationSpec, symbol, decl->templateSpec, decl->specializationSpec);
			if (symbolIsParent != symbolIsChild)
			{
				if (symbolIsParent)
				{
					childCandidates.Add(descendant);
				}
				else
				{
					parentCandidates.Add(descendant);
				}
			}
		}

		KeepDirectRelationship(parentCandidates, &Symbol::GetPSParents_NF);
		KeepDirectRelationship(childCandidates, &Symbol::GetPSChildren_NF);

		for (vint i = 0; i < parentCandidates.Count(); i++)
		{
			auto parent = parentCandidates[i];
			symbol->AddPSParent_NF(parent);

			for (vint j = 0; j < childCandidates.Count(); j++)
			{
				auto child = childCandidates[j];
				child->RemovePSParent_NF(parent);
			}
		}

		for (vint j = 0; j < childCandidates.Count(); j++)
		{
			auto child = childCandidates[j];
			child->AddPSParent_NF(symbol);
		}
	}

	/***********************************************************************
	AssignPSPrimaryInternal
	***********************************************************************/

	template<typename TDecl>
	void AssignPSPrimaryInternal(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, Symbol* symbol)
	{
		if (!symbol->GetPSPrimary_NF())
		{
			auto decl = symbol->GetAnyForwardDecl<TDecl>();
			auto candidates = symbol->GetParentScope()->TryGetChildren_NFb(decl->name.name);
			if (!candidates) throw StopParsingException(cursor);
			for (vint i = 0; i < candidates->Count(); i++)
			{
				auto candidate = candidates->Get(i).Obj();
				if (candidate->kind != symbol_component::SymbolKind::FunctionSymbol)
				{
					symbol->AssignPSPrimary_NF(candidate);
					AssignPSParent<TDecl>(pa, symbol, candidate);
					break;
				}
			}
		}
	}

	/***********************************************************************
	AssignPSPrimary
	***********************************************************************/

	void AssignPSPrimary(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, Symbol* symbol)
	{
		switch (symbol->kind)
		{
		case CLASS_SYMBOL_KIND:
			AssignPSPrimaryInternal<ForwardClassDeclaration>(pa, cursor, symbol);
			break;
		case symbol_component::SymbolKind::ValueAlias:
			AssignPSPrimaryInternal<ValueAliasDeclaration>(pa, cursor, symbol);
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