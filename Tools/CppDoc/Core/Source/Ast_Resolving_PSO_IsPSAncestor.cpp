#include "Ast_Resolving_PSO.h"
#include "Ast_Resolving_AP.h"

using namespace symbol_type_resolving;
using namespace infer_function_type;
using namespace assign_parameters;

namespace partial_specification_ordering
{
	/***********************************************************************
	IsPSAncestor: Test if A is an ancestor of B in partial specification ordering
	***********************************************************************/

	bool IsPSAncestor(
		const ParsingArguments& pa,
		Symbol* symbolA,
		Ptr<TemplateSpec> tA,
		Ptr<SpecializationSpec> psA,
		Symbol* symbolB,
		Ptr<TemplateSpec> tB,
		Ptr<SpecializationSpec> psB
	)
	{
		auto ensureMatched = [&](vint a, vint b)
		{
			switch (tA->arguments[a].argumentType)
			{
			case CppTemplateArgumentType::HighLevelType:
			case CppTemplateArgumentType::Type:
				if (psB->arguments[b].item.expr) throw MatchPSFailureException();
				break;
			case CppTemplateArgumentType::Value:
				if (psB->arguments[b].item.type) throw MatchPSFailureException();
				break;
			}
		};

		if (symbolA == symbolB) return true;
		if (psA)
		{
			if (psB)
			{
				// ensure that they have the same primary symbol
				if (symbolA->GetPSPrimary_NF() != symbolB->GetPSPrimary_NF()) throw TypeCheckerException();

				// fail if both of them are full specialization
				if (tA->arguments.Count() == 0 && tB->arguments.Count() == 0) return false;

				SortedList<Symbol*> freeAncestorSymbols, freeChildSymbols;
				FillFreeSymbols(pa, tA, freeAncestorSymbols);
				FillFreeSymbols(pa, tB, freeChildSymbols);

				// match

				try
				{
					MatchPSAncestorArguments(pa, psA.Obj(), psB.Obj(), freeAncestorSymbols, freeChildSymbols);
				}
				catch (const MatchPSFailureException&)
				{
					return false;
				}
				return true;
			}
			else
			{
				return false;
			}
		}
		else
		{
			// A is primary, only test the amount and type/value kind of B
			// psA must be null
			// in tA and tsB, only the last item is allow to be variadic

			vint minA = tA->arguments.Count();
			bool vtaA = minA == 0 ? false : tA->arguments[minA - 1].ellipsis;

			vint minB = psB->arguments.Count();
			bool vtaB = minB == 0 ? false : psB->arguments[minB - 1].isVariadic;

			if (vtaA) minA -= 1;
			if (vtaB) minB -= 1;

			// ensure that the non-variadic type should have type/value kind matched

			vint commonMin = minA < minB ? minA : minB;
			for (vint i = 0; i < commonMin; i++)
			{
				ensureMatched(i, i);
			}

			// ensure that the rest should have both amount and type/value kind matched
			// TODO: consider about default values in the future, use TypeCheckerException for now
			if (minA < minB)
			{
				if (vtaA && vtaB)
				{
					// <A1, As...>
					// <B1, B2, Bs...>
					for (vint i = minA; i <= minB; i++)
					{
						ensureMatched(minA, i);
					}
				}
				else if (vtaA)
				{
					// <A1, As...>
					// <B1, B2>
					for (vint i = minA; i < minB; i++)
					{
						ensureMatched(minA, i);
					}
				}
				else if (vtaB)
				{
					// <A1>
					// <B1, B2, Bs...>
					// argument amount mismatched
					throw TypeCheckerException();
				}
				else
				{
					// <A1>
					// <B1, B2>
					// argument amount mismatched
					throw TypeCheckerException();
				}
			}
			else if (minA > minB)
			{
				if (vtaA && vtaB)
				{
					// <A1, A2, As...>
					// <B1, Bs...>
					for (vint i = minB; i <= minA; i++)
					{
						ensureMatched(i, minB);
					}
				}
				else if (vtaA)
				{
					// <A1, A2, As...>
					// <B1>
					// argument amount mismatched
					throw TypeCheckerException();
				}
				else if (vtaB)
				{
					// <A1, A2>
					// <B1, Bs...>
					for (vint i = minB; i < minA; i++)
					{
						ensureMatched(i, minB);
					}
				}
				else
				{
					// <A1, A2>
					// <B1>
					// argument amount mismatched
					throw TypeCheckerException();
				}
			}
			else
			{
				if (vtaA && vtaB)
				{
					// <A1, As...>
					// <B1, Bs...>
					ensureMatched(minA, minB);
				}
				else if (vtaA)
				{
					// <A1, As...>
					// <B1>
					// don't care extra A arguments
				}
				else if (vtaB)
				{
					// <A1>
					// <B1, Bs...>
					// don't care extra B arguments
				}
				else
				{
					// <A1>
					// <B1>
					// all arguments are varified
				}
			}
		}
		
		return true;
	}
}