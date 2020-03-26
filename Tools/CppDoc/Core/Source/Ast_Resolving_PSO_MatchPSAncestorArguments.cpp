#include "Ast_Resolving_PSO.h"

using namespace infer_function_type;

namespace partial_specification_ordering
{
	/***********************************************************************
	MatchPSAncestorArguments: Match ancestor types with child types, nullptr means value
	***********************************************************************/

	void MatchPSAncestorArguments(
		const ParsingArguments& pa,
		bool& skipped,
		Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
		Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
		VariadicList<Ptr<Type>>& ancestor,
		VariadicList<Ptr<Type>>& child,
		bool insideVariant,
		const SortedList<Symbol*>& freeTypeSymbols
	)
	{
		// TODO: calculate known pack size for each variadic item in ancestor
		// TODO: if there is any variadic item in ancestor that, has unknown pack size, and is not the last item, skip
		// TODO: if there is any variadic item in child that, is not the last item, fail
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
			MatchPSArgument(pa, skipped, matchingResult, matchingResultVta, ancestorType, childType, insideVariant, freeTypeSymbols, involvedTypes, involvedExprs);
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
		SpecializationSpec* ancestor,
		SpecializationSpec* child,
		const SortedList<Symbol*>& freeTypeSymbols
	)
	{
		VariadicList<Ptr<Type>> ancestorTypes, childTypes;
		FillVariadicTypeList(ancestor->arguments, child->arguments, ancestorTypes, childTypes, [](GenericArgument& e) { return e.type; });

		bool skipped = false;
		Dictionary<Symbol*, Ptr<MatchPSResult>> matchingResult;
		Dictionary<Symbol*, Ptr<MatchPSResult>> matchingResultVta;
		MatchPSAncestorArguments(pa, skipped, matchingResult, matchingResultVta, ancestorTypes, childTypes, false, freeTypeSymbols);

		if (matchingResultVta.Count() > 0)
		{
			// someone misses "..." in psA
			throw TypeCheckerException();
		}

		// TODO: retry if there is any skipped matching, fail if a retry result in no addition item in matchingResult
	}

	/***********************************************************************
	MatchPSAncestorArguments (FunctionType)
	***********************************************************************/

	void MatchPSAncestorArguments(
		const ParsingArguments& pa,
		bool& skipped,
		Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
		Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
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
		MatchPSAncestorArguments(pa, skipped, matchingResult, matchingResultVta, ancestorTypes, childTypes, insideVariant, freeTypeSymbols);
	}

	/***********************************************************************
	MatchPSAncestorArguments (GenericType)
	***********************************************************************/

	void MatchPSAncestorArguments(
		const ParsingArguments& pa,
		bool& skipped,
		Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResult,
		Dictionary<Symbol*, Ptr<MatchPSResult>>& matchingResultVta,
		GenericType* ancestor,
		GenericType* child,
		bool insideVariant,
		const SortedList<Symbol*>& freeTypeSymbols
	)
	{
		VariadicList<Ptr<Type>> ancestorTypes, childTypes;
		FillVariadicTypeList(ancestor->arguments, child->arguments, ancestorTypes, childTypes, [](GenericArgument& e) { return e.type; });
		MatchPSAncestorArguments(pa, skipped, matchingResult, matchingResultVta, ancestorTypes, childTypes, insideVariant, freeTypeSymbols);
	}
}