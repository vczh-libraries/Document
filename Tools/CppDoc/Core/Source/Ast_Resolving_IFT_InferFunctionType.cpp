#include "Ast_Resolving_IFT.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	TemplateArgumentPatternToSymbol:	Get the symbol from a type representing a template argument
	***********************************************************************/

	Symbol* TemplateArgumentPatternToSymbol(ITsys* tsys)
	{
		switch (tsys->GetType())
		{
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

	void SetInferredResult(TemplateArgumentContext& taContext, ITsys* pattern, ITsys* type)
	{
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
				{
					if (type->GetType() == TsysType::Any) return;
				}
				break;
			case symbol_component::SymbolKind::GenericValueArgument:
				break;
			default:
				return;
			}

			// if this argument is inferred, it requires the same result if both of them are not any_t
			auto inferred = taContext.arguments.Values()[index];
			if (inferred->GetType() == TsysType::Any)
			{
				taContext.arguments.Set(pattern, type);
			}
			else if (type != inferred)
			{
				throw TypeCheckerException();
			}
		}
	}

	/***********************************************************************
	InferTemplateArgument:	Perform type inferencing for a template argument
	***********************************************************************/

	void InferTemplateArgumentOfComplexType(
		const ParsingArguments& pa,
		Ptr<Type> argumentType,
		bool isVariadic,
		ITsys* assignedTsys,
		TemplateArgumentContext& taContext,
		TemplateArgumentContext& variadicContext,
		const SortedList<Symbol*>& freeTypeSymbols,
		bool exactMatchForParameters
	)
	{
		SortedList<Type*> involvedTypes;
		CollectFreeTypes(argumentType, isVariadic, freeTypeSymbols, involvedTypes);

		// get all affected arguments
		List<ITsys*> vas;
		List<ITsys*> nvas;
		for (vint j = 0; j < involvedTypes.Count(); j++)
		{
			if (auto idType = dynamic_cast<IdType*>(involvedTypes[j]))
			{
				auto patternSymbol = idType->resolving->resolvedSymbols[0];
				auto pattern = EvaluateGenericArgumentSymbol(patternSymbol);
				if (patternSymbol->ellipsis)
				{
					vas.Add(pattern);
				}
				else
				{
					nvas.Add(pattern);
				}
			}
		}

		// infer all affected types to any_t, result will be overrided if more precise types are inferred
		for (vint j = 0; j < vas.Count(); j++)
		{
			SetInferredResult(taContext, vas[j], pa.tsys->Any());
		}
		for (vint j = 0; j < nvas.Count(); j++)
		{
			SetInferredResult(taContext, nvas[j], pa.tsys->Any());
		}

		if (assignedTsys->GetType() != TsysType::Any)
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
						SetInferredResult(taContext, vas[j], init);
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
						InferTemplateArgument(pa, argumentType, assignedTsysItem, taContext, localVariadicContext, freeTypeSymbols, involvedTypes, exactMatchForParameters);
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
						SetInferredResult(taContext, pattern, init);
					}
				}
			}
			else
			{
				// for non-variadic parameter, run the assigned argument
				InferTemplateArgument(pa, argumentType, assignedTsys, taContext, variadicContext, freeTypeSymbols, involvedTypes, exactMatchForParameters);
			}
		}
	}

	/***********************************************************************
	InferTemplateArgumentsForGenericType:	Perform type inferencing for template class offered arguments
	***********************************************************************/

	void InferTemplateArgumentsForGenericType(
		const ParsingArguments& pa,
		GenericType* genericType,
		List<ITsys*>& parameterAssignment,
		TemplateArgumentContext& taContext,
		TemplateArgumentContext& variadicContext,
		const SortedList<Symbol*>& freeTypeSymbols
	)
	{
		for (vint i = 0; i < genericType->arguments.Count(); i++)
		{
			auto argument = genericType->arguments[i];
			// if this is a value argument, skip it
			if (argument.item.type)
			{
				auto assignedTsys = parameterAssignment[i];
				InferTemplateArgumentOfComplexType(pa, argument.item.type, argument.isVariadic, assignedTsys, taContext, variadicContext, freeTypeSymbols, true);
			}
		}
	}

	/***********************************************************************
	InferTemplateArgumentsForFunctionType:	Perform type inferencing for template function offered arguments
	***********************************************************************/

	void InferTemplateArgumentsForFunctionType(
		const ParsingArguments& pa,
		FunctionType* functionType,
		List<ITsys*>& parameterAssignment,
		TemplateArgumentContext& taContext,
		TemplateArgumentContext& variadicContext,
		const SortedList<Symbol*>& freeTypeSymbols,
		bool exactMatchForParameters
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
				InferTemplateArgumentOfComplexType(pa, parameter.item->type, parameter.isVariadic, assignedTsys, taContext, variadicContext, freeTypeSymbols, exactMatchForParameters);
			}
		}
	}

	/***********************************************************************
	InferFunctionType:	Perform type inferencing for template function using both offered template and function arguments
						Ts(*)(X<Ts...>)... or Ts<X<Ts<Y>...>... is not supported, because of nested Ts...
	***********************************************************************/

	void InferFunctionTypeInternal(const ParsingArguments& pa, FunctionType* functionType, List<ITsys*>& parameterAssignment, TemplateArgumentContext& taContext, SortedList<Symbol*>& freeTypeSymbols)
	{
		TemplateArgumentContext unusedVariadicContext;
		InferTemplateArgumentsForFunctionType(pa, functionType, parameterAssignment, taContext, unusedVariadicContext, freeTypeSymbols, false);
		if (unusedVariadicContext.arguments.Count() > 0)
		{
			// someone miss "..." in a function argument
			throw TypeCheckerException();
		}
	}

	Nullable<ExprTsysItem> InferFunctionType(const ParsingArguments& pa, ExprTsysItem functionItem, Array<ExprTsysItem>& argTypes, SortedList<vint>& boundedAnys)
	{
		switch (functionItem.tsys->GetType())
		{
		case TsysType::Function:
			return functionItem;
		case TsysType::LRef:
		case TsysType::RRef:
		case TsysType::CV:
		case TsysType::Ptr:
			return InferFunctionType(pa, { functionItem,functionItem.tsys->GetElement() }, argTypes, boundedAnys);
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

							List<ITsys*>						parameterAssignment;
							TemplateArgumentContext				taContext;
							SortedList<Symbol*>					freeTypeSymbols;

							auto inferPa = pa.AdjustForDecl(gfi.declSymbol, gfi.parentDeclType, false);

							// cannot pass Ptr<FunctionType> to this function since the last filled argument could be variadic
							// known variadic function argument should be treated as separated arguments
							// ParsingArguments need to be adjusted so that we can evaluate each parameter type
							ResolveFunctionParameters(pa, parameterAssignment, functionType.Obj(), argTypes, boundedAnys);

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
									taContext.arguments.Add(pattern, functionItem.tsys->GetParam(i));
								}
							}

							// type inferencing
							InferFunctionTypeInternal(inferPa, functionType.Obj(), parameterAssignment, taContext, freeTypeSymbols);

							// check if there are symbols that has no result
							if (taContext.arguments.Count() != freeTypeSymbols.Count())
							{
								throw TypeCheckerException();
							}

							taContext.symbolToApply = gfi.declSymbol;
							auto& tsys = EvaluateFuncSymbol(inferPa, decl.Obj(), inferPa.parentDeclType, &taContext);
							if (tsys.Count() == 0)
							{
								return {};
							}
							else if (tsys.Count() == 1)
							{
								return { { functionItem,tsys[0] } };
							}
							else
							{
								// unable to handle multiple types
								throw IllegalExprException();
							}
						}
						catch (const TypeCheckerException&)
						{
							return {};
						}
					}
				}
			}
		default:
			if (functionItem.tsys->IsUnknownType())
			{
				return functionItem;
			}
			return {};
		}
	}
}