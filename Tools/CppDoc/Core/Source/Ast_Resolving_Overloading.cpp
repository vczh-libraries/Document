#include "Ast_Resolving.h"
#include "Symbol_Visit.h"
#include "IFT.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	TestFunctionQualifier: Match this pointer's and functions' qualifiers
		Returns: Exact, TrivalConversion, Illegal
	***********************************************************************/

	TypeConv TestFunctionQualifier(TsysCV thisCV, TsysRefType thisRef, const ExprTsysItem& funcType)
	{
		if (funcType.symbol)
		{
			if (auto decl = funcType.symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
			{
				if (auto declType = GetTypeWithoutMemberAndCC(decl->type).Cast<FunctionType>())
				{
					return ::TestFunctionQualifier(thisCV, thisRef, declType);
				}
			}
		}
		return TypeConvCat::Exact;
	}

	/***********************************************************************
	FilterFieldsAndBestQualifiedFunctions: Filter functions by their qualifiers
	***********************************************************************/

	TypeConv FindMinConv(ArrayBase<TypeConv>& funcChoices)
	{
		auto target = TypeConv::Max();
		for (vint i = 0; i < funcChoices.Count(); i++)
		{
			auto candidate = funcChoices[i];
			if (!candidate.anyInvolved && TypeConv::CompareIgnoreAny(candidate, target) == -1)
			{
				target = candidate;
			}
		}
		return target;
	}

	bool IsFunctionAcceptableByMinConv(TypeConv functionConv, TypeConv minConv)
	{
		if (functionConv.cat == TypeConvCat::Illegal) return false;
		if (functionConv.anyInvolved) return true;
		return TypeConv::CompareIgnoreAny(functionConv, minConv) == 0;
	}

	void FilterFunctionByConv(ExprTsysList& funcTypes, ArrayBase<TypeConv>& funcChoices)
	{
		auto target = FindMinConv(funcChoices);
		for (vint i = funcTypes.Count() - 1; i >= 0; i--)
		{
			if (!IsFunctionAcceptableByMinConv(funcChoices[i], target))
			{
				funcTypes.RemoveAt(i);
			}
		}
	}

	void FilterFieldsAndBestQualifiedFunctions(TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes)
	{
		Array<TypeConv> funcChoices(funcTypes.Count());

		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			funcChoices[i] = TestFunctionQualifier(thisCV, thisRef, funcTypes[i]);
		}

		FilterFunctionByConv(funcTypes, funcChoices);
	}

	/***********************************************************************
	FindQualifiedFunctors: Remove everything that are not qualified functors (including functions and operator())
	***********************************************************************/

	void FindQualifiedFunctors(const ParsingArguments& pa, TsysCV thisCV, TsysRefType thisRef, ExprTsysList& funcTypes, bool lookForOp)
	{
		ExprTsysList expandedFuncTypes;
		List<TypeConv> funcChoices;

		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			auto funcType = funcTypes[i];
			auto choice = TestFunctionQualifier(thisCV, thisRef, funcType);

			if (choice.cat != TypeConvCat::Illegal)
			{
				TsysCV cv;
				TsysRefType refType;
				auto entityType = funcType.tsys->GetEntity(cv, refType);

				if ((entityType->GetType() == TsysType::Decl || entityType->GetType() == TsysType::DeclInstant) && lookForOp)
				{
					ExprTsysList opResult;
					VisitFunctors(pa, funcType, L"operator ()", opResult);

					vint oldCount = expandedFuncTypes.Count();
					AddNonVar(expandedFuncTypes, opResult);
					vint newCount = expandedFuncTypes.Count();

					for (vint i = 0; i < (newCount - oldCount); i++)
					{
						funcChoices.Add(TypeConvCat::Exact);
					}
				}
				else if (entityType->GetType() == TsysType::GenericFunction)
				{
					if (auto symbol = funcTypes[i].symbol)
					{
						if (auto decl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
						{
							if (AddInternal(expandedFuncTypes, { funcType.symbol,funcType.type, entityType }))
							{
								funcChoices.Add(choice);
							}
						}
					}
				}
				else if (entityType->GetType() == TsysType::Ptr)
				{
					if (entityType->GetElement()->GetType() == TsysType::Function)
					{
						if (AddInternal(expandedFuncTypes, { funcType.symbol,funcType.type, entityType }))
						{
							funcChoices.Add(choice);
						}
					}
				}
				else if (entityType->IsUnknownType())
				{
					if (AddInternal(expandedFuncTypes, { funcType.symbol,funcType.type,entityType }))
					{
						funcChoices.Add({ TypeConvCat::Exact, false, true });
					}
				}
			}
		}

		FilterFunctionByConv(expandedFuncTypes, funcChoices);
		CopyFrom(funcTypes, expandedFuncTypes);
	}

	/***********************************************************************
	VisitOverloadedFunction: Select good candidates from overloaded functions
	***********************************************************************/

	bool IsAcceptableWithVariadicInput(const ParsingArguments& pa, ExprTsysItem funcItem, Array<ExprTsysItem>& argTypes, SortedList<vint>& boundedAnys)
	{
		return true;
	}

	void VisitOverloadedFunction(
		const ParsingArguments& pa,
		ExprTsysList& funcTypes,
		Array<ExprTsysItem>& argTypes,
		SortedList<vint>& boundedAnys,
		ExprTsysList& result,
		List<ResolvedItem>* ritems,
		bool* anyInvolved
	)
	{
		bool withVariadicInput = boundedAnys.Count() > 0;

		// funcDPs: functionIndex(funcTypes) -> number of parameters with default value
		Array<vint> funcDPs(funcTypes.Count());
		for (vint i = 0; i < funcTypes.Count(); i++)
		{
			funcDPs[i] = 0;
			if (auto symbol = funcTypes[i].symbol)
			{
				if (auto decl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
				{
					if (auto type = GetTypeWithoutMemberAndCC(decl->type).Cast<FunctionType>())
					{
						for (vint j = 0; j < type->parameters.Count(); j++)
						{
							if (type->parameters[j].item->initializer)
							{
								funcDPs[i] = type->parameters.Count() - j;
								break;
							}
						}
					}
				}
			}
		}

		bool addedAny = false;
		ExprTsysList inferredFunctionTypes;
		Group<vint, ResolvedItem> gritems;
		List<vint> validIndices;
		List<vint> inferredToAnyIndices;

		if (withVariadicInput)
		{
			// if number of arguments is unknown, we only check if a function has too few parameters.
			for (vint i = 0; i < funcTypes.Count(); i++)
			{
				vint originalCount = inferredFunctionTypes.Count();
				infer_function_type::InferFunctionType(pa, inferredFunctionTypes, funcTypes[i], argTypes, boundedAnys, (ritems ? &gritems : nullptr));
				for (vint j = originalCount; j < inferredFunctionTypes.Count(); j++)
				{
					auto funcType = inferredFunctionTypes[j];
					if (funcType.tsys->IsUnknownType())
					{
						if (!addedAny)
						{
							addedAny = true;
							AddType(result, pa.tsys->Any());
						}
						inferredToAnyIndices.Add(j);
						continue;
					}

					if (funcType.tsys->GetParamCount() < argTypes.Count() - boundedAnys.Count())
					{
						if (!funcType.tsys->GetFunc().ellipsis)
						{
							continue;
						}
					}
					validIndices.Add(j);
				}
			}
		}
		else
		{
			for (vint i = 0; i < funcTypes.Count(); i++)
			{
				vint originalCount = inferredFunctionTypes.Count();
				infer_function_type::InferFunctionType(pa, inferredFunctionTypes, funcTypes[i], argTypes, boundedAnys, (ritems ? &gritems : nullptr));
				for (vint j = originalCount; j < inferredFunctionTypes.Count(); j++)
				{
					auto funcType = inferredFunctionTypes[j];
					if (funcType.tsys->IsUnknownType())
					{
						if (!addedAny)
						{
							addedAny = true;
							AddType(result, pa.tsys->Any());
						}
						inferredToAnyIndices.Add(j);
						continue;
					}

					vint funcParamCount = funcType.tsys->GetParamCount();
					vint missParamCount = funcParamCount - argTypes.Count();
					if (missParamCount > 0)
					{
						// if arguments are not enough, we check about parameters with default value
						if (missParamCount > funcDPs[i])
						{
							continue;
						}
					}
					else if (missParamCount < 0)
					{
						// if arguments is too many, we check about ellipsis
						if (!funcType.tsys->GetFunc().ellipsis)
						{
							continue;
						}
					}

					validIndices.Add(j);
				}
			}
		}

		Array<bool> selectedIndices(validIndices.Count());
		Array<bool> anyInvolveds(validIndices.Count());

		if (withVariadicInput)
		{
			// if number of arguments is unknown, we perform an inaccurate test, only filtering away a few inappropriate candidates
			for (vint i = 0; i < validIndices.Count(); i++)
			{
				selectedIndices[i] = IsAcceptableWithVariadicInput(pa, inferredFunctionTypes[validIndices[i]], argTypes, boundedAnys);
				anyInvolveds[i] = false;
			}
		}
		else
		{
			// if number of arguments is known, we need to pick the best candidate
			for (vint i = 0; i < validIndices.Count(); i++)
			{
				selectedIndices[i] = true;
				anyInvolveds[i] = false;
			}

			vint minLoopCount = argTypes.Count();
			if (minLoopCount < 1) minLoopCount = 1;

			for (vint i = 0; i < minLoopCount; i++)
			{
				Array<TypeConv> funcChoices(validIndices.Count());

				for (vint j = 0; j < validIndices.Count(); j++)
				{
					auto funcType = inferredFunctionTypes[validIndices[j]];

					// best candidates for this arguments are chosen
					if (i < argTypes.Count())
					{
						vint funcParamCount = funcType.tsys->GetParamCount();
						if (funcParamCount <= i)
						{
							funcChoices[j] = TypeConvCat::Ellipsis;
							continue;
						}

						auto paramType = funcType.tsys->GetParam(i);
						funcChoices[j] = TestTypeConversion(pa, paramType, argTypes[i]);
					}
					else
					{
						// if there is not argument, we say functions without ellipsis are better
						funcChoices[j] = funcType.tsys->GetFunc().ellipsis ? TypeConvCat::Ellipsis : TypeConvCat::Exact;
					}
				}

				// the best candidate should be the best in for all arguments at the same time
				auto min = FindMinConv(funcChoices);
				for (vint j = 0; j < validIndices.Count(); j++)
				{
					auto choice = funcChoices[j];
					if (IsFunctionAcceptableByMinConv(choice, min))
					{
						if (choice.anyInvolved)
						{
							anyInvolveds[j] = true;
							if (anyInvolved)
							{
								*anyInvolved = choice.anyInvolved;
							}
						}
					}
					else
					{
						selectedIndices[j] = false;
					}
				}
			}
		}

		if(!withVariadicInput && validIndices.Count() > 0)
		{
			// prefer non-template functions over template functions
			// prefer non-variadic argument over variadic argument
			// prefer non-ellipsis over ellipsis
			// always select a function when any_t conversion is involved in its overloading resolution

			const vint Normal = 0;
			const vint Ellipsis = 1;
			const vint Template = 2;
			const vint Variadic = 3;
			const vint Others = 4;
			const vint AnyInvolved = 5;
			const vint Unselected = 6;

			Array<vint> priorities(validIndices.Count());

			for (vint i = 0; i < selectedIndices.Count(); i++)
			{
				if (!selectedIndices[i])
				{
					priorities[i] = Unselected;
				}
				else if (anyInvolveds[i])
				{
					priorities[i] = AnyInvolved;
				}
				else
				{
					auto symbol = inferredFunctionTypes[validIndices[i]].symbol;
					if (!symbol)
					{
						priorities[i] = Others;
					}
					else if (auto funcDecl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
					{
						if (!funcDecl->templateSpec)
						{
							if (GetTypeWithoutMemberAndCC(funcDecl->type).Cast<FunctionType>()->ellipsis)
							{
								priorities[i] = Ellipsis;
							}
							else
							{
								priorities[i] = Normal;
							}
						}
						else
						{
							// find the primary function if it is a full specialized function
							if (symbol->GetCategory() == symbol_component::SymbolCategory::FunctionBody)
							{
								symbol = symbol->GetFunctionSymbol_Fb();
							}

							if (auto primary = symbol->GetPSPrimary_NF())
							{
								primary = symbol;
							}

							funcDecl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>();
							auto funcType = GetTypeWithoutMemberAndCC(funcDecl->type).Cast<FunctionType>();
							if (funcType->parameters.Count() == 0)
							{
								priorities[i] = Template;
							}
							else if (funcType->parameters[funcType->parameters.Count() - 1].isVariadic)
							{
								priorities[i] = Variadic;
							}
							else
							{
								priorities[i] = Template;
							}
						}
					}
					else
					{
						priorities[i] = Others;
					}
				}
			}

			vint min = From(priorities).Min();
			if (min != Unselected)
			{
				for (vint i = 0; i < selectedIndices.Count(); i++)
				{
					selectedIndices[i] = priorities[i] == min || priorities[i] == AnyInvolved;
				}
			}
		}

		// fill function types
		for (vint i = 0; i < selectedIndices.Count(); i++)
		{
			if (selectedIndices[i])
			{
				AddTempValue(result, inferredFunctionTypes[validIndices[i]].tsys->GetElement());
			}
		}

		if (ritems)
		{
			// fill indexing
			for (vint i = 0; i < selectedIndices.Count(); i++)
			{
				if (selectedIndices[i])
				{
					vint index = gritems.Keys().IndexOf(validIndices[i]);
					if (index == -1)
					{
						ResolvedItem::AddItem(*ritems, { nullptr,inferredFunctionTypes[validIndices[i]].symbol });
					}
					else
					{
						auto& gvitems = gritems.GetByIndex(index);
						ResolvedItem::AddItems(*ritems, gvitems);
					}
				}
			}

			for (vint i = 0; i < inferredToAnyIndices.Count(); i++)
			{
				ResolvedItem::AddItem(*ritems, { nullptr,inferredFunctionTypes[inferredToAnyIndices[i]].symbol });
			}
		}
	}
}