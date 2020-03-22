#include "Ast_Resolving_AP.h"
#include "Ast_Evaluate_ExpandPotentialVta.h"

using namespace symbol_type_resolving;

namespace infer_function_type
{
	/***********************************************************************
	TemplateArgumentPatternToSymbol:	Get the symbol from a type representing a template argument
	***********************************************************************/

	Symbol* TemplateArgumentPatternToSymbol(ITsys* tsys)
	{
		switch (tsys->GetType())
		{
		case TsysType::Decl:
			return tsys->GetDecl();
		case TsysType::GenericArg:
			return tsys->GetGenericArg().argSymbol;
		case TsysType::GenericFunction:
			{
				auto symbol = tsys->GetGenericFunction().declSymbol;
				if (symbol->kind != symbol_component::SymbolKind::GenericTypeArgument)
				{
					throw TypeCheckerException();
				}
				return symbol;
			}
		default:
			throw TypeCheckerException();
		}
	}

	/***********************************************************************
	SetInferredResult:	Set a inferred type for a template argument, and check if it is compatible with previous result
	***********************************************************************/

	void SetInferredResult(const ParsingArguments& pa, TemplateArgumentContext& taContext, ITsys* pattern, ITsys* type, ITsys** lastAssignedVta, SortedList<ITsys*>& hardcodedPatterns)
	{
		bool hitLastAssignedVta = lastAssignedVta && *lastAssignedVta == pattern;
		if (hitLastAssignedVta && type->GetType() != TsysType::Any)
		{
			*lastAssignedVta = nullptr;
		}

		vint index = taContext.arguments.Keys().IndexOf(pattern);
		if (index == -1)
		{
			// if this argument is not inferred, use the result
			taContext.arguments.Add(pattern, type);
		}
		else
		{
			switch (TemplateArgumentPatternToSymbol(pattern)->kind)
			{
			case symbol_component::SymbolKind::GenericTypeArgument:
			case symbol_component::SymbolKind::GenericValueArgument:
				{
					if (type && type->GetType() == TsysType::Any) return;
				}
				break;
			default:
				return;
			}

			// if this argument is inferred, it requires the same result if both of them are not any_t
			// unless this is the first assignment to the last variadic template argument, the previously inferred type could be extended
			auto inferred = taContext.arguments.Values()[index];
			if (inferred->GetType() == TsysType::Any)
			{
				taContext.arguments.Set(pattern, type);
			}
			else if (type != inferred)
			{
				if (hitLastAssignedVta)
				{
					// check if the new type extends the old type
					if (type && type->GetType() == TsysType::Init && inferred->GetType() == TsysType::Init)
					{
						if (type->GetParamCount() >= inferred->GetParamCount())
						{
							for (vint i = 0; i < inferred->GetParamCount(); i++)
							{
								if (type->GetParam(i) != inferred->GetParam(i))
								{
									if (!hardcodedPatterns.Contains(pattern))
									{
										// only fail when the pattern is not hardcoded.
										// because there could be implicit type conversion happens later in function overloading resolution.
										throw TypeCheckerException();
									}
									else
									{
										if (type->GetParamCount() > inferred->GetParamCount())
										{
											// keep the hardcoded prefix.
											Array<ExprTsysItem> params(type->GetParamCount());
											for (vint i = 0; i < inferred->GetParamCount(); i++)
											{
												params[i] = { inferred->GetInit().headers[i],inferred->GetParam(i) };
											}
											for (vint i = inferred->GetParamCount(); i < type->GetParamCount(); i++)
											{
												params[i] = { type->GetInit().headers[i],type->GetParam(i) };
											}

											auto init = pa.tsys->InitOf(params);
											taContext.arguments.Set(pattern, type);
										}
										return;
									}
								}
							}
							taContext.arguments.Set(pattern, type);
							return;
						}
						throw TypeCheckerException();
					}
				}

				if (!hardcodedPatterns.Contains(pattern))
				{
					// only fail when the pattern is not hardcoded.
					// because there could be implicit type conversion happens later in function overloading resolution.
					throw TypeCheckerException();
				}
			}
		}
	}

	/***********************************************************************
	InferTemplateArgument:	Perform type inferencing for a template argument
	***********************************************************************/

	void InferTemplateArgumentOfComplexType(
		const ParsingArguments& pa,
		Ptr<Type> typeToInfer,
		Ptr<Expr> exprToInfer,
		bool isVariadic,
		ITsys* assignedTsys,
		TemplateArgumentContext& taContext,
		TemplateArgumentContext& variadicContext,
		const SortedList<Symbol*>& freeTypeSymbols,
		bool exactMatchForParameters,
		ITsys** lastAssignedVta,
		SortedList<ITsys*>& hardcodedPatterns
	)
	{
		SortedList<Type*> involvedTypes;
		SortedList<Expr*> involvedExprs;
		CollectFreeTypes(pa, false, typeToInfer, exprToInfer, isVariadic, freeTypeSymbols, involvedTypes, involvedExprs);

		// get all affected arguments
		TypeTsysList vas;
		CollectInvolvedVariadicArguments(pa, involvedTypes, involvedExprs, [&vas](Symbol*, ITsys* pattern)
		{
			vas.Add(pattern);
		});

		// infer all affected types to any_t, result will be overrided if more precise types are inferred
		for (vint j = 0; j < vas.Count(); j++)
		{
			SetInferredResult(pa, taContext, vas[j], pa.tsys->Any(), nullptr, hardcodedPatterns);
		}

		if (!assignedTsys || assignedTsys->GetType() != TsysType::Any)
		{
			if (isVariadic)
			{
				// for variadic parameter
				vint count = assignedTsys->GetParamCount();
				if (count == 0)
				{
					// if the assigned argument is an empty list, infer all variadic arguments to empty
					Array<ExprTsysItem> params;
					auto init = pa.tsys->InitOf(params);
					for (vint j = 0; j < vas.Count(); j++)
					{
						SetInferredResult(pa, taContext, vas[j], init, lastAssignedVta, hardcodedPatterns);
					}
				}
				else
				{
					// if the assigned argument is a non-empty list
					Dictionary<ITsys*, Ptr<Array<ExprTsysItem>>> variadicResults;
					for (vint j = 0; j < vas.Count(); j++)
					{
						variadicResults.Add(vas[j], MakePtr<Array<ExprTsysItem>>(count));
					}

					// run each item in the list
					for (vint j = 0; j < count; j++)
					{
						auto assignedTsysItem = ApplyExprTsysType(assignedTsys->GetParam(j), assignedTsys->GetInit().headers[j].type);
						TemplateArgumentContext localVariadicContext;
						// set lastAssignedVta = nullptr, since now the element of the type list is to be inferred, so that localVariadicContext doesn't allow conflict
						InferTemplateArgument(pa, typeToInfer, exprToInfer, assignedTsysItem, taContext, localVariadicContext, freeTypeSymbols, involvedTypes, involvedExprs, exactMatchForParameters, nullptr, hardcodedPatterns);
						for (vint k = 0; k < localVariadicContext.arguments.Count(); k++)
						{
							auto key = localVariadicContext.arguments.Keys()[k];
							auto value = localVariadicContext.arguments.Values()[k];
							auto result = variadicResults[key];
							result->Set(j, { nullptr,ExprTsysType::PRValue,value });
						}
					}

					// aggregate them
					for (vint j = 0; j < vas.Count(); j++)
					{
						auto pattern = vas[j];
						auto& params = *variadicResults[pattern].Obj();
						auto init = pa.tsys->InitOf(params);
						SetInferredResult(pa, taContext, pattern, init, lastAssignedVta, hardcodedPatterns);
					}
				}
			}
			else
			{
				// for non-variadic parameter, run the assigned argument
				InferTemplateArgument(pa, typeToInfer, exprToInfer, assignedTsys, taContext, variadicContext, freeTypeSymbols, involvedTypes, involvedExprs, exactMatchForParameters, lastAssignedVta, hardcodedPatterns);
			}
		}
	}

	/***********************************************************************
	InferTemplateArgumentsForGenericType:	Perform type inferencing for template class offered arguments
	***********************************************************************/

	void InferTemplateArgumentsForGenericType(
		const ParsingArguments& pa,
		GenericType* genericType,
		TypeTsysList& parameterAssignment,
		TemplateArgumentContext& taContext,
		TemplateArgumentContext& variadicContext,
		const SortedList<Symbol*>& freeTypeSymbols,
		ITsys** lastAssignedVta,
		SortedList<ITsys*>& hardcodedPatterns
	)
	{
		for (vint i = 0; i < genericType->arguments.Count(); i++)
		{
			auto argument = genericType->arguments[i];
			auto assignedTsys = parameterAssignment[i];
			InferTemplateArgumentOfComplexType(pa, argument.item.type, argument.item.expr, argument.isVariadic, assignedTsys, taContext, variadicContext, freeTypeSymbols, true, lastAssignedVta, hardcodedPatterns);
		}
	}

	/***********************************************************************
	InferTemplateArgumentsForFunctionType:	Perform type inferencing for template function offered arguments
	***********************************************************************/

	void InferTemplateArgumentsForFunctionType(
		const ParsingArguments& pa,
		FunctionType* functionType,
		TypeTsysList& parameterAssignment,
		TemplateArgumentContext& taContext,
		TemplateArgumentContext& variadicContext,
		const SortedList<Symbol*>& freeTypeSymbols,
		bool exactMatchForParameters,
		ITsys** lastAssignedVta,
		SortedList<ITsys*>& hardcodedPatterns
	)
	{
		// don't care about arguments for ellipsis
		for (vint i = 0; i < functionType->parameters.Count(); i++)
		{
			// if default value is used, skip it
			if (auto assignedTsys = parameterAssignment[i])
			{
				// see if any variadic value arguments can be determined
				// variadic value argument only care about the number of values
				auto parameter = functionType->parameters[i];
				InferTemplateArgumentOfComplexType(pa, parameter.item->type, nullptr, parameter.isVariadic, assignedTsys, taContext, variadicContext, freeTypeSymbols, exactMatchForParameters, lastAssignedVta, hardcodedPatterns);
			}
		}
	}

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
		if (!genericType->type->resolving) return false;
		if (genericType->type->resolving->resolvedSymbols.Count() != 1) return false;

		auto classSymbol = genericType->type->resolving->resolvedSymbols[0];
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
			auto current = visited[i];
			switch (current->GetType())
			{
			case TsysType::Decl:
				{
					// Decl could not match GenericType, search for base classes
					if (auto classDecl = current->GetDecl()->GetAnyForwardDecl<ClassDeclaration>())
					{
						auto& ev = EvaluateClassSymbol(pa, classDecl.Obj(), nullptr, nullptr);
						for (vint j = 0; j < ev.ExtraCount(); j++)
						{
							CopyFrom(visited, ev.GetExtra(j), true);
						}
					}
				}
				break;
			case TsysType::DeclInstant:
				{
					auto& di = current->GetDeclInstant();
					if (di.declSymbol == classSymbol)
					{
						if (i == 0)
						{
							// if the parameter is an instance of an expected template class, no conversion is needed
							return false;
						}
						else
						{
							// otherwise, add the current type and stop searching for base classes
							mbcTsys.Add(CvRefOf(current, cv, ref));
						}
					}
					else if (auto classDecl = di.declSymbol->GetAnyForwardDecl<ClassDeclaration>())
					{
						// search for base classes
						auto& ev = EvaluateClassSymbol(pa, classDecl.Obj(), di.parentDeclType, di.taContext.Obj());
						for (vint j = 0; j < ev.ExtraCount(); j++)
						{
							CopyFrom(visited, ev.GetExtra(j), true);
						}
					}
				}
				break;
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
					auto tac = MakePtr<TemplateArgumentContext>();
					tac->parent = taContext.parent;
					CopyFrom(tac->arguments, taContext.arguments);

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

					TemplateArgumentContext unusedVariadicContext;
					try
					{
						InferTemplateArgumentsForFunctionType(pa, functionType, assignment, *tac.Obj(), unusedVariadicContext, freeTypeSymbols, false, lastAssignedVta, hardcodedPatterns);
						inferredArgumentTypes.Add(tac);
					}
					catch (const TypeCheckerException&)
					{
						// ignore this candidate if failed to match
					}
					if (unusedVariadicContext.arguments.Count() > 0)
					{
						// someone miss "..." in a function argument
						throw TypeCheckerException();
					}
				});
		}
		else
		{
			auto tac = MakePtr<TemplateArgumentContext>();
			tac->parent = taContext.parent;
			CopyFrom(tac->arguments, taContext.arguments);

			TemplateArgumentContext unusedVariadicContext;
			InferTemplateArgumentsForFunctionType(pa, functionType, parameterAssignment, *tac.Obj(), unusedVariadicContext, freeTypeSymbols, false, lastAssignedVta, hardcodedPatterns);
			inferredArgumentTypes.Add(tac);

			if (unusedVariadicContext.arguments.Count() > 0)
			{
				// someone miss "..." in a function argument
				throw TypeCheckerException();
			}
		}
	}

	void InferFunctionType(const ParsingArguments& pa, ExprTsysList& inferredFunctionTypes, ExprTsysItem functionItem, Array<ExprTsysItem>& argTypes, SortedList<vint>& boundedAnys)
	{
		switch (functionItem.tsys->GetType())
		{
		case TsysType::Function:
			if (auto source = functionItem.tsys->GetFunc().genericSource)
			{
				InferFunctionType(pa, inferredFunctionTypes, { functionItem,source }, argTypes, boundedAnys);
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
			InferFunctionType(pa, inferredFunctionTypes, { functionItem,functionItem.tsys->GetElement() }, argTypes, boundedAnys);
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
							TemplateArgumentContext				taContext;
							SortedList<Symbol*>					freeTypeSymbols;
							ITsys*								lastAssignedVta = nullptr;
							SortedList<ITsys*>					hardcodedPatterns;

							// fill freeTypeSymbols with all template arguments
							// fill taContext will knows arguments
							for (vint i = 0; i < gfi.spec->arguments.Count(); i++)
							{
								auto argument = gfi.spec->arguments[i];
								auto pattern = GetTemplateArgumentKey(argument, pa.tsys.Obj());
								auto patternSymbol = TemplateArgumentPatternToSymbol(pattern);
								freeTypeSymbols.Add(patternSymbol);

								if (i < gfi.filledArguments)
								{
									hardcodedPatterns.Add(pattern);
									taContext.arguments.Add(pattern, functionItem.tsys->GetParam(i));
									if (i == gfi.spec->arguments.Count() - 1 && argument.ellipsis)
									{
										lastAssignedVta = pattern;
									}
								}
							}

							// assign arguments to correct parameters
							auto inferPa = pa.AdjustForDecl(gfi.declSymbol);
							inferPa.parentDeclType = ParsingArguments::AdjustDeclInstantForScope(gfi.declSymbol, gfi.parentDeclType, true);
							assign_parameters::ResolveFunctionParameters(pa, parameterAssignment, taContext, freeTypeSymbols, lastAssignedVta, functionType.Obj(), argTypes, boundedAnys);

							// type inferencing
							List<Ptr<TemplateArgumentContext>> inferredArgumentTypes;
							InferFunctionTypeInternal(inferPa, inferredArgumentTypes, functionType.Obj(), parameterAssignment, taContext, freeTypeSymbols, &lastAssignedVta, hardcodedPatterns);

							for (vint i = 0; i < inferredArgumentTypes.Count(); i++)
							{
								// skip all incomplete inferrings
								auto tac = inferredArgumentTypes[i];
								if (tac->arguments.Count() == freeTypeSymbols.Count())
								{
									tac->symbolToApply = gfi.declSymbol;
									auto& tsys = EvaluateFuncSymbol(inferPa, decl.Obj(), inferPa.parentDeclType, tac.Obj());
									for (vint j = 0; j < tsys.Count(); j++)
									{
										inferredFunctionTypes.Add({ functionItem,tsys[j] });
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