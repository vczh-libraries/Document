#include "Ast_Resolving_PSO.h"

using namespace infer_function_type;

namespace partial_specification_ordering
{
	/***********************************************************************
	MatchPSAncestorArguments: Match ancestor types with child types, nullptr means value
	***********************************************************************/

	void MatchPSAncestorArguments(
		const ParsingArguments& pa,
		const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
		const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
		VariadicList<Ptr<Type>>& ancestor,
		VariadicList<Ptr<Type>>& child,
		bool insideVariant,
		const SortedList<Symbol*>& freeTypeSymbols
	)
	{
		// TODO: support variadic template argument pack
		for (vint i = 0; i < ancestor.Count(); i++) if (ancestor[i].isVariadic) throw 0;
		for (vint i = 0; i < child.Count(); i++) if (child[i].isVariadic) throw 0;
		if (ancestor.Count() != child.Count()) throw 0;

		for (vint i = 0; i < ancestor.Count(); i++)
		{
			auto ancestorType = ancestor[i].item;
			auto childType = child[i].item;
			SortedList<Type*> involvedTypes;
			SortedList<Expr*> involvedExprs;
			CollectFreeTypes(pa, false, ancestorType, nullptr, insideVariant, freeTypeSymbols, involvedTypes, involvedExprs);
			MatchPSArgument(pa, matchingResult, matchingResultVta, ancestorType, childType, insideVariant, freeTypeSymbols, involvedTypes, involvedExprs);
		}
	}

	template<typename TSource, typename TGetter>
	void FillVariadicTypeList(VariadicList<TSource>& ancestor, VariadicList<TSource>& child, VariadicList<Ptr<Type>>& ancestorTypes, VariadicList<Ptr<Type>>& childTypes, TGetter&& getter)
	{
		for (vint i = 0; i < ancestor.Count(); i++)
		{
			auto& e = ancestor[i];
			ancestorTypes.Add({ getter(e.item),e.isVariadic });
		}

		for (vint i = 0; i < child.Count(); i++)
		{
			auto& e = child[i];
			childTypes.Add({ getter(e.item),e.isVariadic });
		}
	}

	/***********************************************************************
	MatchPSAncestorArguments (SpecializationSpec)
	***********************************************************************/

	void MatchPSAncestorArguments(
		const ParsingArguments& pa,
		const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
		const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
		SpecializationSpec* ancestor,
		SpecializationSpec* child,
		const SortedList<Symbol*>& freeTypeSymbols
	)
	{
		VariadicList<Ptr<Type>> ancestorTypes, childTypes;
		FillVariadicTypeList(ancestor->arguments, child->arguments, ancestorTypes, childTypes, [](GenericArgument& e) { return e.type; });
		MatchPSAncestorArguments(pa, matchingResult, matchingResultVta, ancestorTypes, childTypes, false, freeTypeSymbols);

		// retry if some arguments cannot resolve because of <unknown-pack..., y...>
	}

	/***********************************************************************
	MatchPSAncestorArguments (FunctionType)
	***********************************************************************/

	void MatchPSAncestorArguments(
		const ParsingArguments& pa,
		const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
		const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
		FunctionType* ancestor,
		FunctionType* child,
		bool insideVariant,
		const SortedList<Symbol*>& freeTypeSymbols
	)
	{
		if (ancestor->ellipsis != child->ellipsis)
		{
			throw MatchPSFailureException();
		}

		// TODO: take care about T[] and T* for parameters
		VariadicList<Ptr<Type>> ancestorTypes, childTypes;
		FillVariadicTypeList(ancestor->parameters, child->parameters, ancestorTypes, childTypes, [](Ptr<VariableDeclaration>& e) { return e->type; });
		MatchPSAncestorArguments(pa, matchingResult, matchingResultVta, ancestorTypes, childTypes, insideVariant, freeTypeSymbols);
	}

	/***********************************************************************
	MatchPSAncestorArguments (GenericType)
	***********************************************************************/

	void MatchPSAncestorArguments(
		const ParsingArguments& pa,
		const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
		const Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
		GenericType* ancestor,
		GenericType* child,
		bool insideVariant,
		const SortedList<Symbol*>& freeTypeSymbols
	)
	{
		VariadicList<Ptr<Type>> ancestorTypes, childTypes;
		FillVariadicTypeList(ancestor->arguments, child->arguments, ancestorTypes, childTypes, [](GenericArgument& e) { return e.type; });
		MatchPSAncestorArguments(pa, matchingResult, matchingResultVta, ancestorTypes, childTypes, insideVariant, freeTypeSymbols);
	}
}