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
		const SortedList<Symbol*>& freeAncestorSymbols,
		const SortedList<Symbol*>& freeChildSymbols
	)
	{
		// calculate known pack size for each variadic item in ancestor
		// skip of there are more than one variadic item in ancestor with unknown pack size
		// fail if there are more than one variadic item in child
		// only match when
		//   <A1, A2, A3, A4>		with <B1, B2, B3, B4>
		//   <A1, As..., A2>		with <B1, B2, B3, B4>		A1={B2, B3}[0,0]
		//   <A1, A2, A3, A4>		with <B1, Bs..., B2>		A2=Bs[0,1]; A3=Bs[1,0]
		//   <A1, A2, As...>		with <B1, B2, B3, Bs...>	As={B3, Bs...}[0,0]
		//   <A1, A2, A3, As...>	with <B1, B2, Bs...>		A3=Bs[0,-1]; As={Bs}[1,0]

		vint ancestorVta = -1;
		vint childVta = -1;

		for (vint i = 0; i < ancestor.Count(); i++)
		{
			if (ancestor[i].isVariadic)
			{
				if (ancestorVta == -1)
				{
					ancestorVta = i;
				}
				else
				{
					throw MatchPSFailureException();
				}
			}
		}

		for (vint i = 0; i < child.Count(); i++)
		{
			if (child[i].isVariadic)
			{
				if (childVta == -1)
				{
					childVta = i;
				}
				else
				{
					throw MatchPSFailureException();
				}
			}
		}

		if (ancestorVta == -1)
		{
			if (childVta == -1)
			{
				// there is no variadic item on both ancestor and child
				if (ancestor.Count() != child.Count()) throw MatchPSFailureException();
			}
			else
			{
				// only child has variadic item
				if (ancestor.Count() < child.Count() - 1) throw MatchPSFailureException();
			}
		}
		else
		{
			if (childVta == -1)
			{
				// only ancestor has variadic item
				if (ancestor.Count() - 1 > child.Count()) throw MatchPSFailureException();
			}
			else
			{
				// both ancestor and child have variadic item
				if (ancestorVta != ancestor.Count() - 1) throw MatchPSFailureException();
				if (childVta != child.Count() - 1) throw MatchPSFailureException();
			}
		}

		for (vint i = 0; i < ancestor.Count(); i++)
		{
			auto ancestorType = ancestor[i].item;
			SortedList<Type*> involvedTypes;
			SortedList<Expr*> involvedExprs;
			CollectFreeTypes(pa, false, ancestorType, nullptr, insideVariant, freeAncestorSymbols, involvedTypes, involvedExprs);

			// ancestorType is non-variadic
			// child[c] is non-variadic
			// match ancestorType with child[c]
			auto matchSingleToSingle = [&](vint c)
			{
				auto childType = child[c].item;
				MatchPSArgument(pa, skipped, matchingResult, matchingResultVta, ancestorType, childType, insideVariant, freeAncestorSymbols, freeChildSymbols, involvedTypes, involvedExprs);
			};

			// ancestorType is non-variadic
			// child[c] is variadic
			// match ancestorType with the start-th item in child[c]
			auto matchSingleToVariadicPart = [&](vint c, vint start)
			{
				throw 0;
			};

			// ancestorType is variadic
			// child[start] to child[stop-1]
			// when withExtraVariadicItem == true, child[stop] is variadic
			// match ancestorType with all of mentioned items in child above
			auto matchVariadicToMultipleSingle = [&](vint start, vint stop, bool withExtraVariadicItem)
			{
				throw 0;
			};

			// ancestorType is variadic
			// child[c] is variadic
			// match ancestorType with the postfix of child[c] starting at start
			auto matchVariantToVariadicPart = [&](vint c, vint start)
			{
				throw 0;
			};

			// ancestorType is variadic
			// child[c] is variadic
			// match ancestorType with child[c]
			auto matchVariadicToVariadic = [&](vint c)
			{
				throw 0;
			};

			if (ancestorVta == -1)
			{
				if (childVta == -1)
				{
					// there is no variadic item on both ancestor and child
					matchSingleToSingle(i);
				}
				else
				{
					// only child has variadic item
					auto childPostfix = child.Count() - childVta - 1;
					if (i < childVta)
					{
						// match ancestor item with child item in prefix
						matchSingleToSingle(i);
					}
					else if (ancestor.Count() - i <= childPostfix)
					{
						// match ancestor item with child item in postfix
						matchSingleToSingle(i + (child.Count() - ancestor.Count()));
					}
					else
					{
						// match ancestor item with one item in child variadic item
						matchSingleToVariadicPart(childVta, i - childVta);
					}
				}
			}
			else
			{
				if (childVta == -1)
				{
					// only ancestor has variadic item
					auto ancestorPostfix = ancestor.Count() - ancestorVta - 1;
					if (i < ancestorVta)
					{
						// match ancestor item with child item in prefix
						matchSingleToSingle(i);
					}
					else if (ancestor.Count() - i <= ancestorPostfix)
					{
						// match ancestor item with child item in postfix
						matchSingleToSingle(i + (child.Count() - ancestor.Count()));
					}
					else
					{
						// match ancestor variadic item with multiple child item
						matchVariadicToMultipleSingle(ancestorVta, child.Count() - ancestorPostfix, false);
					}
				}
				else
				{
					// both ancestor and child have variadic item
					if (i < ancestorVta && i < childVta)
					{
						// match child item in prefix
						matchSingleToSingle(i);
					}
					else if (ancestorVta < childVta)
					{
						// match ancestor variadic item with multiple child item and the child variadic item
						matchVariadicToMultipleSingle(ancestorVta, childVta, true);
					}
					else if (ancestorVta > childVta)
					{
						if (i < ancestorVta)
						{
							// match ancestor item with one item in child variadic item
							matchSingleToVariadicPart(childVta, i - childVta);
						}
						else
						{
							// match ancestor variadic item with a postfix of child variadic item
							matchVariantToVariadicPart(childVta, ancestorVta - childVta);
						}
					}
					else
					{
						// match ancestor variadic item with child variadic item
						matchVariadicToVariadic(ancestorVta);
					}
				}
			}
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
		const SortedList<Symbol*>& freeAncestorSymbols,
		const SortedList<Symbol*>& freeChildSymbols
	)
	{
		VariadicList<Ptr<Type>> ancestorTypes, childTypes;
		FillVariadicTypeList(ancestor->arguments, child->arguments, ancestorTypes, childTypes, [](GenericArgument& e) { return e.type; });

		bool skipped = false;
		Dictionary<Symbol*, Ptr<MatchPSResult>> matchingResult;
		Dictionary<Symbol*, Ptr<MatchPSResult>> matchingResultVta;
		MatchPSAncestorArguments(pa, skipped, matchingResult, matchingResultVta, ancestorTypes, childTypes, false, freeAncestorSymbols, freeChildSymbols);

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
		const SortedList<Symbol*>& freeAncestorSymbols,
		const SortedList<Symbol*>& freeChildSymbols
	)
	{
		if (ancestor->ellipsis != child->ellipsis)
		{
			throw MatchPSFailureException();
		}

		// TODO: take care about T[] and T* for parameters
		VariadicList<Ptr<Type>> ancestorTypes, childTypes;
		FillVariadicTypeList(ancestor->parameters, child->parameters, ancestorTypes, childTypes, [](Ptr<VariableDeclaration>& e) { return e->type; });
		MatchPSAncestorArguments(pa, skipped, matchingResult, matchingResultVta, ancestorTypes, childTypes, insideVariant, freeAncestorSymbols, freeChildSymbols);
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
		const SortedList<Symbol*>& freeAncestorSymbols,
		const SortedList<Symbol*>& freeChildSymbols
	)
	{
		VariadicList<Ptr<Type>> ancestorTypes, childTypes;
		FillVariadicTypeList(ancestor->arguments, child->arguments, ancestorTypes, childTypes, [](GenericArgument& e) { return e.type; });
		MatchPSAncestorArguments(pa, skipped, matchingResult, matchingResultVta, ancestorTypes, childTypes, insideVariant, freeAncestorSymbols, freeChildSymbols);
	}
}