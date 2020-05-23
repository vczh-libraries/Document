#include "PSO.h"
#include "AP.h"
#include "IFT.h"
#include "Parser.h"

using namespace symbol_type_resolving;
using namespace assign_parameters;
using namespace infer_function_type;

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
					removed.Add(s);
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
		// add the primary symbol to parent list
		List<Symbol*> parentCandidates, childCandidates;
		parentCandidates.Add(symbol->GetPSPrimary_NF());

		auto decl = symbol->GetAnyForwardDecl<TDecl>();
		auto& descendants = primary->GetPSPrimaryDescendants_NF();

		// find all potential parents and children
		for (vint i = 0; i < descendants.Count(); i++)
		{
			auto descendant = descendants[i];
			if (symbol != descendant)
			{
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
		}

		// remove all indirect relationship
		KeepDirectRelationship(parentCandidates, &Symbol::GetPSParents_NF);
		KeepDirectRelationship(childCandidates, &Symbol::GetPSChildren_NF);

		// adjust parent-child records
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
	bool CheckPSPrimary(const ParsingArguments& pa, Ptr<TDecl> decl, Symbol* symbol, Symbol* primary)
	{
		// the primary symbol of a class or value alias will never be a function
		if (primary->kind != symbol_component::SymbolKind::FunctionSymbol)
		{
			symbol->AssignPSPrimary_NF(primary);
			AssignPSParent<TDecl>(pa, symbol, primary);
			return true;
		}
		return false;
	}

	template<>
	bool CheckPSPrimary<ForwardFunctionDeclaration>(const ParsingArguments& pa, Ptr<ForwardFunctionDeclaration> decl, Symbol* symbol, Symbol* primary)
	{
		if (primary->kind == symbol_component::SymbolKind::FunctionSymbol)
		{
			// only the last template argument of the primary function can be variant
			auto primaryDecl = primary->GetAnyForwardDecl<ForwardFunctionDeclaration>();
			auto tspec = primaryDecl->templateSpec;
			if (!tspec) return false;
			if (tspec->arguments.Count() == 0) return false;
			for (vint i = 0; i < tspec->arguments.Count() - 1; i++)
			{
				if (tspec->arguments[i].ellipsis) return false;
			}

			// check argument count
			auto sspec = decl->specializationSpec;
			bool vta = tspec->arguments[tspec->arguments.Count() - 1].ellipsis;
			if (vta)
			{
				if (sspec->arguments.Count() < tspec->arguments.Count() - 1) return false;
			}
			else
			{
				if (sspec->arguments.Count() != tspec->arguments.Count()) return false;
			}

			// assign decl->specializationSpec to primaryDecl->templateSpec
			Dictionary<Symbol*, Ptr<MatchPSResult>> matchingResult, matchingResultVta;
			for (vint i = 0; i < tspec->arguments.Count(); i++)
			{
				auto targ = tspec->arguments[i];
				auto pattern = GetTemplateArgumentKey(targ);
				auto patternSymbol = TemplateArgumentPatternToSymbol(pattern);
				auto result = MakePtr<MatchPSResult>();
				matchingResult.Add(patternSymbol, result);

				vint smin = i;
				vint smax = i;
				if (targ.ellipsis)
				{
					// has already ensured that targ is the last one
					smax = sspec->arguments.Count() - 1;
				}

				for (vint j = smin; j <= smax; j++)
				{
					auto sarg = sspec->arguments[j];
					switch (targ.argumentType)
					{
					case CppTemplateArgumentType::HighLevelType:
					case CppTemplateArgumentType::Type:
						if (!sarg.item.type) return false;
						result->source.Add(sarg.item.type);
						break;
					case CppTemplateArgumentType::Value:
						if (!sarg.item.expr) return false;
						result->source.Add(nullptr);
						break;
					}
				}
			}

			// match function type
			auto ptype = GetTypeWithoutMemberAndCC(primaryDecl->type).Cast<FunctionType>();
			auto stype = GetTypeWithoutMemberAndCC(decl->type).Cast<FunctionType>();
			bool skipped = false;
			SortedList<Symbol*> freeAncestorSymbols, freeChildSymbols;
			SortedList<Type*> involvedTypes;
			SortedList<Expr*> involvedExprs;

			FillFreeSymbols(pa, tspec, freeAncestorSymbols);
			CollectFreeTypes(pa, false, ptype, nullptr, false, freeAncestorSymbols, involvedTypes, involvedExprs);

			try
			{
				MatchPSArgument(pa, skipped, matchingResult, matchingResultVta, { ptype,nullptr }, { stype,nullptr }, false, freeAncestorSymbols, freeChildSymbols, involvedTypes, involvedExprs, false);
				if (!skipped)
				{
					// function cannot be partial specialization
					symbol->AssignPSPrimary_NF(primary);
					symbol->AddPSParent_NF(primary);
					return true;
				}
			}
			catch (const MatchPSFailureException&)
			{
			}
		}
		return false;
	}

	template<typename TDecl>
	void AssignPSPrimaryInternal(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, Symbol* symbol)
	{
		// only the first call to a symbol takes effect
		if (!symbol->GetPSPrimary_NF())
		{
			// symbol names of partial specializations are decorated
			// so searching for the declaration name will only return the primary symbol
			// or sometimes some overloading symbols (e.g. function and struct with the same name)
			auto decl = symbol->GetAnyForwardDecl<TDecl>();
			auto candidates = symbol->GetParentScope()->TryGetChildren_NFb(decl->name.name);
			if (!candidates) throw StopParsingException(cursor);
			for (vint i = 0; i < candidates->Count(); i++)
			{
				auto& candidate = candidates->Get(i);
				if (!candidate.parentDeclType)
				{
					if (CheckPSPrimary<TDecl>(pa, decl, symbol, candidate.childSymbol.Obj()))
					{
						break;
					}
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
			AssignPSPrimaryInternal<ForwardFunctionDeclaration>(pa, cursor, symbol);
			break;
		default:
			throw StopParsingException(cursor);
		}
	}
}