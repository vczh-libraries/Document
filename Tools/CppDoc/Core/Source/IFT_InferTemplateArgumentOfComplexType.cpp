#include "IFT.h"
#include "AP.h"

using namespace symbol_type_resolving;
using namespace assign_parameters;

namespace infer_function_type
{
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
			else if (!IsPSEquivalentType(pa, type, inferred))
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
			if (!vas.Contains(pattern))
			{
				vas.Add(pattern);
			}
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
						TemplateArgumentContext localVariadicContext(&taContext, false);
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
	InferTemplateArgumentsForSpecializationSpec:	Perform type inferencing for template class offered arguments
	***********************************************************************/

	void InferTemplateArgumentsForSpecializationSpec(
		const ParsingArguments& pa,
		SpecializationSpec* spec,
		TypeTsysList& parameterAssignment,
		TemplateArgumentContext& taContext,
		TemplateArgumentContext& variadicContext,
		const SortedList<Symbol*>& freeTypeSymbols,
		ITsys** lastAssignedVta,
		SortedList<ITsys*>& hardcodedPatterns
	)
	{
		for (vint i = 0; i < spec->arguments.Count(); i++)
		{
			auto argument = spec->arguments[i];
			auto assignedTsys = parameterAssignment[i];
			InferTemplateArgumentOfComplexType(pa, argument.item.type, argument.item.expr, argument.isVariadic, assignedTsys, taContext, variadicContext, freeTypeSymbols, true, lastAssignedVta, hardcodedPatterns);
		}
	}
}