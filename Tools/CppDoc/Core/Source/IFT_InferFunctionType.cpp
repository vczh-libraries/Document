#include "IFT.h"
#include "AP.h"
#include "Ast_Evaluate_ExpandPotentialVta.h"
#include "EvaluateSymbol.h"

using namespace symbol_type_resolving;
using namespace assign_parameters;

namespace infer_function_type
{
	/***********************************************************************
	InferFunctionType:	Perform type inferencing for template function using both offered template and function arguments
						Ts(*)(X<Ts...>)... or Ts<X<Ts<Y>...>... is not supported, because of nested Ts...
	***********************************************************************/

	struct MatchBaseClassRecord
	{
		vint				parameterIndex = -1;	// function parameter index
		vint				variadicIndex = -1;		// -1 for non-variadic function parameter, index in Init type for others
		vint				start = -1;				// index in TypeTsysList
		vint				count = 1;				// 1 for non-variadic function parameter, number of items in TypeTsysList for others
	};

	static bool operator==(const MatchBaseClassRecord& a, const MatchBaseClassRecord& b)
	{
		return a.parameterIndex == b.parameterIndex
			&& a.variadicIndex == b.variadicIndex
			&& a.start == b.start
			&& a.count == b.count
			;
	}

	bool CreateMbcr(const ParsingArguments& pa, Ptr<Type>& type, ITsys* value, MatchBaseClassRecord& mbcr, TypeTsysList& mbcTsys)
	{
		// for default value
		if (!value) return false;

		TsysCV cv;
		TsysRefType ref;
		auto entityValue = value->GetEntity(cv, ref);
		if (entityValue->GetType() != TsysType::Decl && entityValue->GetType() != TsysType::DeclInstant)
		{
			return false;
		}

		auto entityType = type;

		if (auto refType = entityType.Cast<ReferenceType>())
		{
			if (refType->reference != CppReferenceType::Ptr)
			{
				entityType = refType->type;
			}
		}

		if (auto cvType = entityType.Cast<DecorateType>())
		{
			entityType = cvType->type;
		}

		auto genericType = entityType.Cast<GenericType>();
		if (!genericType) return false;

		auto classSymbol = Resolving::EnsureSingleSymbol(genericType->type->resolving);
		if (!classSymbol) return false;

		switch (classSymbol->kind)
		{
		case CLASS_SYMBOL_KIND:
			break;
		default:
			return false;
		}

		TypeTsysList visited;
		visited.Add(entityValue);

		mbcr.start = mbcTsys.Count();
		for (vint i = 0; i < visited.Count(); i++)
		{
			bool exitWithFalse = false;
			bool found = false;
			auto current = visited[i];

			EnumerateClassPrimaryInstances(pa, current, false, [&](ITsys* currentPrimary)
			{
				switch (currentPrimary->GetType())
				{
				case TsysType::DeclInstant:
					{
						auto& di = currentPrimary->GetDeclInstant();
						if (di.declSymbol == classSymbol)
						{
							if (i == 0)
							{
								// if the parameter is an instance of an expected template class, no conversion is needed
								exitWithFalse = true;
								return true;
							}
							else
							{
								// otherwise, add the current type and stop searching for base classes
								mbcTsys.Add(CvRefOf(currentPrimary, cv, ref));
								found = true;
							}
						}
					}
					break;
				}
				return false;
			});

			if (exitWithFalse)
			{
				return false;
			}

			// even if we use the primary symbol to match, we still use current to evaluate base classes, if it is an instance of a partial specialization class
			if (!found)
			{
				switch (current->GetDecl()->kind)
				{
				case CLASS_SYMBOL_KIND:
					{
						ClassDeclaration* cd = nullptr;
						ITsys* pdt = nullptr;
						TemplateArgumentContext* ata = nullptr;
						ExtractClassType(current, cd, pdt, ata);
						symbol_type_resolving::EnumerateClassSymbolBaseTypes(pa, cd, pdt, ata, [&](ITsys* classType, ITsys* baseType)
						{
							if (!visited.Contains(baseType))
							{
								visited.Add(baseType);
							}
							return false;
						});
					}
					break;
				}
			}
		}

		mbcr.count = mbcTsys.Count() - mbcr.start;
		return mbcr.count > 0;
	}

	void InferFunctionTypeInternal(
		const ParsingArguments& pa,
		List<Ptr<TemplateArgumentContext>>& inferredArgumentTypes,
		FunctionType* functionType,
		TypeTsysList& parameterAssignment,
		TemplateArgumentContext& taContext,
		SortedList<Symbol*>& freeTypeSymbols,
		ITsys** lastAssignedVta,
		SortedList<ITsys*>& hardcodedPatterns
	)
	{
		List<MatchBaseClassRecord> mbcs;
		TypeTsysList mbcTsys;

		for (vint i = 0; i < functionType->parameters.Count(); i++)
		{
			auto argument = functionType->parameters[i];
			if (argument.isVariadic)
			{
				if (auto value = parameterAssignment[i])
				{
					if (value->GetType() == TsysType::Init)
					{
						for (vint j = 0; j < value->GetParamCount(); j++)
						{
							MatchBaseClassRecord mbcr;
							mbcr.parameterIndex = i;
							mbcr.variadicIndex = j;
							if (CreateMbcr(pa, argument.item->type, value->GetParam(j), mbcr, mbcTsys))
							{
								mbcs.Add(mbcr);
							}
						}
					}
				}
			}
			else
			{
				MatchBaseClassRecord mbcr;
				mbcr.parameterIndex = i;
				if (CreateMbcr(pa, argument.item->type, parameterAssignment[i], mbcr, mbcTsys))
				{
					mbcs.Add(mbcr);
				}
			}
		}

		if (mbcs.Count() > 0)
		{
			// create a fake ending mbcr so that there is no out of range accessing
			{
				MatchBaseClassRecord mbcr;
				mbcr.parameterIndex = -1;
				mbcs.Add(mbcr);
			}

			vint parameterCount = 0;
			Array<vint> vtaCounts(functionType->parameters.Count());
			for (vint i = 0; i < functionType->parameters.Count(); i++)
			{
				if (functionType->parameters[i].isVariadic && parameterAssignment[i] && parameterAssignment[i]->GetType() == TsysType::Init)
				{
					vint count = parameterAssignment[i]->GetParamCount();
					vtaCounts[i] = count;
					parameterCount += count;
				}
				else
				{
					vtaCounts[i] = -1;
					parameterCount++;
				}
			}

			Array<TypeTsysList> inputs(parameterCount);
			Array<bool> isVtas(parameterCount);
			vint index = 0;
			vint currentMbcr = 0;

			for (vint i = 0; i < parameterCount; i++)
			{
				isVtas[i] = false;
			}

			for (vint i = 0; i < vtaCounts.Count(); i++)
			{
				if (vtaCounts[i] == -1)
				{
					auto& mbcr = mbcs[currentMbcr];
					if (mbcr.parameterIndex == i)
					{
						currentMbcr++;
						for (vint k = 0; k < mbcr.count; k++)
						{
							inputs[index].Add(mbcTsys[mbcr.start + k]);
						}
					}
					else
					{
						inputs[index].Add(parameterAssignment[i]);
					}
					index++;
				}
				else
				{
					auto init = parameterAssignment[i];
					for (vint j = 0; j < init->GetParamCount(); j++)
					{
						auto& mbcr = mbcs[currentMbcr];
						if (mbcr.parameterIndex == i && mbcr.variadicIndex == j)
						{
							currentMbcr++;
							for (vint k = 0; k < mbcr.count; k++)
							{
								inputs[index].Add(mbcTsys[mbcr.start + k]);
							}
						}
						else
						{
							inputs[index].Add(init->GetParam(j));
						}
						index++;
					}
				}
			}

			if (index != parameterCount || currentMbcr != mbcs.Count() - 1)
			{
				// something is wrong
				throw IllegalExprException();
			}

			ExprTsysList unused;
			symbol_totsys_impl::ExpandPotentialVtaList(pa, unused, inputs, isVtas, false, -1,
				[&](ExprTsysList&, Array<ExprTsysItem>& params, vint, Array<vint>&, SortedList<vint>&)
				{
					auto tac = Ptr(new TemplateArgumentContext(taContext, true));

					List<ITsys*> assignment;
					vint index = 0;
					for (vint i = 0; i < vtaCounts.Count(); i++)
					{
						if (vtaCounts[i] == -1)
						{
							assignment.Add(params[index++].tsys);
						}
						else
						{
							Array<ExprTsysItem> initParams(vtaCounts[i]);
							for (vint j = 0; j < initParams.Count(); j++)
							{
								initParams[j] = params[index++];
							}
							auto init = pa.tsys->InitOf(params);
							assignment.Add(init);
						}
					}

					if (index != params.Count())
					{
						// something is wrong
						throw IllegalExprException();
					}

					TemplateArgumentContext unusedVariadicContext(nullptr, 0);
					try
					{
						InferTemplateArgumentsForFunctionType(pa, functionType, assignment, *tac.Obj(), unusedVariadicContext, freeTypeSymbols, false, lastAssignedVta, hardcodedPatterns);
						inferredArgumentTypes.Add(tac);
					}
					catch (const TypeCheckerException&)
					{
						// ignore this candidate if failed to match
					}
				});
		}
		else
		{
			auto tac = Ptr(new TemplateArgumentContext(taContext, true));

			TemplateArgumentContext unusedVariadicContext(nullptr, 0);
			InferTemplateArgumentsForFunctionType(pa, functionType, parameterAssignment, *tac.Obj(), unusedVariadicContext, freeTypeSymbols, false, lastAssignedVta, hardcodedPatterns);
			inferredArgumentTypes.Add(tac);
		}
	}

	void InferFunctionType(
		const ParsingArguments& pa,
		ExprTsysList& inferredFunctionTypes,
		ExprTsysItem functionItem,
		Array<ExprTsysItem>& argTypes,
		SortedList<vint>& boundedAnys,
		Group<vint, ResolvedItem>* gritems
	)
	{
		switch (functionItem.tsys->GetType())
		{
		case TsysType::Function:
			if (auto source = functionItem.tsys->GetFunc().genericSource)
			{
				InferFunctionType(pa, inferredFunctionTypes, { functionItem,source }, argTypes, boundedAnys, gritems);
			}
			else
			{
				inferredFunctionTypes.Add(functionItem);
			}
			break;
		case TsysType::LRef:
		case TsysType::RRef:
		case TsysType::CV:
		case TsysType::Ptr:
			InferFunctionType(pa, inferredFunctionTypes, { functionItem,functionItem.tsys->GetElement() }, argTypes, boundedAnys, gritems);
			break;
		case TsysType::GenericFunction:
			if (auto symbol = functionItem.symbol)
			{
				if (auto decl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
				{
					if (auto functionType = GetTypeWithoutMemberAndCC(decl->type).Cast<FunctionType>())
					{
						try
						{
							auto gfi = functionItem.tsys->GetGenericFunction();

							TypeTsysList						parameterAssignment;
							TemplateArgumentContext				taContext(gfi.declSymbol, functionItem.tsys->GetParamCount());
							SortedList<Symbol*>					freeTypeSymbols;
							ITsys*								lastAssignedVta = nullptr;
							SortedList<ITsys*>					hardcodedPatterns;

							// fill freeTypeSymbols with all template arguments
							// fill taContext will knows arguments
							FillFreeSymbols(pa, gfi.spec, freeTypeSymbols, [&](vint i, TemplateSpec::Argument& argument, ITsys* pattern, Symbol* patternSymbol)
							{
								if (i < gfi.filledArguments)
								{
									hardcodedPatterns.Add(pattern);
									taContext.SetValueByKey(pattern, functionItem.tsys->GetParam(i));
									if (i == gfi.spec->arguments.Count() - 1 && argument.ellipsis)
									{
										lastAssignedVta = pattern;
									}
								}
							});

							// assign arguments to correct parameters
							auto inferPa = pa.AdjustForDecl(gfi.declSymbol);
							if (inferPa.taContext && inferPa.taContext->GetSymbolToApply() == gfi.declSymbol)
							{
								inferPa.taContext = inferPa.taContext->parent;
							}
							inferPa.parentDeclType = ParsingArguments::AdjustDeclInstantForScope(gfi.declSymbol, gfi.parentDeclType, true);
							ResolveFunctionParameters(inferPa, parameterAssignment, taContext, freeTypeSymbols, lastAssignedVta, functionType.Obj(), argTypes, boundedAnys);

							// type inferencing
							List<Ptr<TemplateArgumentContext>> inferredArgumentTypes;
							InferFunctionTypeInternal(inferPa, inferredArgumentTypes, functionType.Obj(), parameterAssignment, taContext, freeTypeSymbols, &lastAssignedVta, hardcodedPatterns);

							for (vint i = 0; i < inferredArgumentTypes.Count(); i++)
							{
								// skip all incomplete inferrings
								auto tac = inferredArgumentTypes[i];
								if (tac->GetAvailableArgumentCount() == freeTypeSymbols.Count())
								{
									List<ResolvedItem> ritems;
									auto& tsys = EvaluateFuncSymbol(inferPa, decl.Obj(), inferPa.parentDeclType, tac.Obj(), (gritems ? &ritems : nullptr));
									for (vint j = 0; j < tsys.Count(); j++)
									{
										vint index = inferredFunctionTypes.Add({ functionItem,tsys[j] });
										if (ritems.Count() > 0)
										{
											for (vint k = 0; k < ritems.Count(); k++)
											{
												ResolvedItem ritem = ritems[k];
												if (!gritems->Contains(index, ritem))
												{
													gritems->Add(index, ritem);
												}
											}
										}
									}
								}
							}
						}
						catch (const TypeCheckerException&)
						{
							// ignore this candidate if failed to match
						}
						break;
					}
				}
			}
		default:
			if (functionItem.tsys->IsUnknownType())
			{
				inferredFunctionTypes.Add(functionItem);
			}
		}
	}
}