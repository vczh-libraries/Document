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
		VariadicList<GenericArgument>& ancestor,
		VariadicList<GenericArgument>& child,
		bool insideVariant,
		const SortedList<Symbol*>& freeAncestorSymbols,
		const SortedList<Symbol*>& freeChildSymbols,
		bool forParameter
	)
	{
		vint ancestorVta = -1;
		Array<vint> ancestorPackSizes(ancestor.Count());

		for (vint i = 0; i < ancestor.Count(); i++)
		{
			if (ancestor[i].isVariadic)
			{
				vint packSize = -1;

				// calculate known pack size for each variadic item in ancestor
				auto ancestorItem = ancestor[i].item;
				SortedList<Type*> involvedTypes;
				SortedList<Expr*> involvedExprs;
				CollectFreeTypes(pa, false, ancestorItem.type, ancestorItem.expr, insideVariant, freeAncestorSymbols, involvedTypes, involvedExprs);

				SortedList<Symbol*> variadicSymbols;
				CollectInvolvedVariadicArguments(pa, involvedTypes, involvedExprs, [&](Symbol* patternSymbol, ITsys*)
				{
					vint index = matchingResult.Keys().IndexOf(patternSymbol);
					if (index != -1)
					{
						vint newPackSize = matchingResult.Values()[index]->source.Count();
						if (packSize == -1)
						{
							packSize = newPackSize;
						}
						else if(packSize != newPackSize)
						{
							// fail if pack size conflicts
							throw MatchPSFailureException();
						}
					}
				});

				ancestorPackSizes[i] = packSize;
				if (packSize == -1)
				{
					if (ancestorVta == -1)
					{
						ancestorVta = i;
					}
					else
					{
						// skip of there are more than one variadic item in ancestor with unknown pack size
						skipped = true;
						return;
					}
				}
			}
			else
			{
				ancestorPackSizes[i] = 1;
			}
		}

		// only match when
		//   <A1, A2, A3, A4>		with <B1, B2, B3, B4>
		//   <A1, As..., A2>		with <B1, B2, B3, B4>		A1={B2, B3}[-1]
		//   <A1, A2, As...>		with <B1, B2, B3, Bs...>	As={B3, Bs...}[1]
		//   <A1, A2, As...>		with <B1, B2, Bs..., B3>	As={Bs..., B3}[0]
		// followings are not supported
		//   <A1, A2, A3, A4>		with <B1, Bs..., B2>
		//   <A1, A2, A3, As...>	with <B1, B2, Bs...>

		struct Assignment
		{
			vint			start;
			vint			stop;

			Assignment()
			{
			}

			Assignment(vint _start, vint _stop)
				:start(_start)
				, stop(_stop)
			{
			}
		};

		Array<Assignment> assignments(ancestor.Count());

		if (ancestorVta == -1)
		{
			// there is no variadic item with unknown pack size in ancestor
			// the sum of ancestor item pack sizes should match the amount of child items

			vint nextChild = 0;
			for (vint i = 0; i < ancestor.Count(); i++)
			{
				vint newNextChild = nextChild + ancestorPackSizes[i];
				assignments[i] = { nextChild,newNextChild };
				nextChild = newNextChild;
			}
			if (nextChild != child.Count()) throw MatchPSFailureException();
		}
		else
		{
			// there is one variadic item with unknown pack size in ancestor
			// the sum of prefix and postfix ancestor item pack sizes should not exceed the amount of child items

			vint nextChildFromLeft = 0;
			vint nextChildFromRight = child.Count();

			for (vint i = 0; i < ancestorVta; i++)
			{
				vint newNextChild = nextChildFromLeft + ancestorPackSizes[i];
				assignments[i] = { nextChildFromLeft,newNextChild };
				nextChildFromLeft = newNextChild;
			}

			for (vint i = ancestor.Count() - 1; i > ancestorVta; i--)
			{
				vint newNextChild = nextChildFromRight - ancestorPackSizes[i];
				assignments[i] = { newNextChild,nextChildFromRight };
				nextChildFromRight = newNextChild;
			}

			if (nextChildFromLeft > nextChildFromRight) throw MatchPSFailureException();

			assignments[ancestorVta] = { nextChildFromLeft,nextChildFromRight };

		}

		// any ancestor item should not match part of variadic child item
		// any non-variadic ancestor item should not match variadic child item

		for (vint i = 0; i < ancestor.Count(); i++)
		{
			auto assignment = assignments[i];
			if (!ancestor[i].isVariadic)
			{
				if (child[assignment.start].isVariadic)
				{
					throw MatchPSFailureException();
				}
			}
		}

		// match ancestor items according to assignments
		for (vint i = 0; i < ancestor.Count(); i++)
		{
			// search for all variadic symbols in this ancestor item
			auto ancestorItem = ancestor[i].item;
			SortedList<Type*> involvedTypes;
			SortedList<Expr*> involvedExprs;
			CollectFreeTypes(pa, false, ancestorItem.type, ancestorItem.expr, insideVariant, freeAncestorSymbols, involvedTypes, involvedExprs);

			SortedList<Symbol*> variadicSymbols;
			CollectInvolvedVariadicArguments(pa, involvedTypes, involvedExprs, [&](Symbol* patternSymbol, ITsys*)
			{
				if (!variadicSymbols.Contains(patternSymbol))
				{
					variadicSymbols.Add(patternSymbol);
				}
			});

			auto matchSingleToSingleComplex = [&](vint c, Dictionary<Symbol*, Ptr<MatchPSResult>>& replacedResultVta)
			{
				auto childItem = child[c].item;
				MatchPSArgument(pa, skipped, matchingResult, replacedResultVta, ancestorItem, childItem, child[c].isVariadic, freeAncestorSymbols, freeChildSymbols, involvedTypes, involvedExprs, forParameter);
			};

			auto assertMatchingResultPass1 = [&](vint assignedCount)
			{
				for (vint i = 0; i < variadicSymbols.Count(); i++)
				{
					vint index = matchingResult.Keys().IndexOf(variadicSymbols[i]);
					if (index != -1)
					{
						auto assigned = matchingResult.Values()[index];
						if (assigned->source.Count() != assignedCount)
						{
							throw MatchPSFailureException();
						}
					}
				}
			};

			Dictionary<WString, WString> equivalentNames;
			auto assertMatchingResultPass2 = [&](vint assignedCount, vint matchIndex, Dictionary<Symbol*, Ptr<MatchPSResult>>& replacedResultVta)
			{
				for (vint i = 0; i < variadicSymbols.Count(); i++)
				{
					auto variadicSymbol = variadicSymbols[i];
					vint resultIndex = replacedResultVta.Keys().IndexOf(variadicSymbol);
					if (resultIndex == -1) throw MatchPSFailureException();
					auto resultType = replacedResultVta.Values()[resultIndex]->source[0];

					Ptr<MatchPSResult> result;
					vint index = matchingResult.Keys().IndexOf(variadicSymbol);
					if (index == -1)
					{
						result = MakePtr<MatchPSResult>();
						matchingResult.Add(variadicSymbol, result);
					}
					else
					{
						result = matchingResult.Values()[index];
					}

					if (result->source.Count() == assignedCount)
					{
						auto matchType = result->source[matchIndex];
						if (matchType && resultType)
						{
							if (!IsSameResolvedType(matchType, resultType, equivalentNames))
							{
								throw MatchPSFailureException();
							}
						}
						else if (matchType || resultType)
						{
							throw MatchPSFailureException();
						}
					}
					else
					{
						result->source.Add(resultType);
					}
				}
			};

			// ancestorType is non-variadic
			// child[c] is non-variadic
			// match ancestorType with child[c]
			auto matchSingleToSingle = [&](vint c)
			{
				matchSingleToSingleComplex(c, matchingResultVta);
			};

			// ancestorType is variadic
			// child[start] to child[stop-1]
			// when variadicItemIndex != -1, child[start + variadicItemIndex] is variadic
			// match ancestorType with all of mentioned items in child above
			auto matchVariadicToMultiple = [&](vint start, vint stop)
			{
				vint assignedCount = stop - start;
				assertMatchingResultPass1(assignedCount);

				for (vint c = start; c < stop; c++)
				{
					Dictionary<Symbol*, Ptr<MatchPSResult>> replacedResultVta;
					matchSingleToSingleComplex(c, replacedResultVta);
					assertMatchingResultPass2(assignedCount, c - start, replacedResultVta);
				}
			};

			auto assignment = assignments[i];
			if (ancestor[i].isVariadic)
			{
				matchVariadicToMultiple(assignment.start, assignment.stop);
			}
			else
			{
				matchSingleToSingle(assignment.start);
			}
		}
	}
	/***********************************************************************
	FixFunctionParameterType: Change pointer type to array type without the dimension expression
	***********************************************************************/

	Ptr<Type> FixFunctionParameterType(Ptr<Type> t)
	{
		// convert all T* to T[]
		// the reason not to do the reverse is that
		// sometimes T[here] could contain variadic template value argument
		if (auto pointerType = t.Cast<ReferenceType>())
		{
			if (pointerType->reference == CppReferenceType::Ptr)
			{
				auto arrayType = MakePtr<ArrayType>();
				arrayType->type = pointerType->type;
				return arrayType;
			}
		}
		return t;
	}

	template<typename TSource, typename TGetter>
	void FillVariadicTypeList(
		VariadicList<TSource>& items,
		VariadicList<GenericArgument>& types,
		bool forParameters,
		TGetter&& getter
	)
	{
		for (vint i = 0; i < items.Count(); i++)
		{
			auto& e = items[i];
			auto ga = getter(e.item);
			if (forParameters)
			{
				ga.type = FixFunctionParameterType(ga.type);
			}
			types.Add({ ga,e.isVariadic });
		}
	}

	template<typename TSource, typename TGetter>
	void FillVariadicTypeList(
		VariadicList<TSource>& ancestor,
		VariadicList<TSource>& child,
		VariadicList<GenericArgument>& ancestorTypes,
		VariadicList<GenericArgument>& childTypes,
		bool forParameters,
		TGetter&& getter
	)
	{
		FillVariadicTypeList(ancestor, ancestorTypes, forParameters, ForwardValue<TGetter&&>(getter));
		FillVariadicTypeList(child, childTypes, forParameters, ForwardValue<TGetter&&>(getter));
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
		VariadicList<GenericArgument> ancestorTypes, childTypes;
		FillVariadicTypeList(
			ancestor->arguments,
			child->arguments,
			ancestorTypes,
			childTypes,
			false,
			[](GenericArgument& e) { return e; }
		);

		// retry if there is any skipped matching
		// fail if a retry result in no addition item in matchingResult

		Dictionary<Symbol*, Ptr<MatchPSResult>> matchingResult;
		Dictionary<Symbol*, Ptr<MatchPSResult>> matchingResultVta;
		while (true)
		{
			bool skipped = false;
			vint lastCount = matchingResult.Count();
			MatchPSAncestorArguments(pa, skipped, matchingResult, matchingResultVta, ancestorTypes, childTypes, false, freeAncestorSymbols, freeChildSymbols, false);
			if (skipped)
			{
				if (lastCount == matchingResult.Count())
				{
					throw MatchPSFailureException();
				}
			}
			else
			{
				if (matchingResultVta.Count() > 0)
				{
					// someone misses "..." in psA
					throw TypeCheckerException();
				}
				break;
			}
		}
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

		VariadicList<GenericArgument> ancestorTypes, childTypes;
		FillVariadicTypeList(
			ancestor->parameters,
			child->parameters,
			ancestorTypes,
			childTypes,
			true,
			[](Ptr<VariableDeclaration>& e)
			{
				GenericArgument ga;
				ga.type = e->type;
				return ga;
			}
		);
		MatchPSAncestorArguments(pa, skipped, matchingResult, matchingResultVta, ancestorTypes, childTypes, insideVariant, freeAncestorSymbols, freeChildSymbols, true);
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
		VariadicList<GenericArgument> ancestorTypes, childTypes;
		FillVariadicTypeList(
			ancestor->arguments,
			child->arguments,
			ancestorTypes,
			childTypes,
			false,
			[](GenericArgument& e) { return e; }
		);
		MatchPSAncestorArguments(pa, skipped, matchingResult, matchingResultVta, ancestorTypes, childTypes, insideVariant, freeAncestorSymbols, freeChildSymbols, false);
	}
}