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

	struct MatchBaseClassRecord
	{
		vint				parameterIndex = -1;	// function parameter index
		vint				variadicIndex = -1;		// -1 for non-variadic function parameter, index in Init type for others
		vint				start = -1;				// index in List<ITsys*>
		vint				count = 1;				// 1 for non-variadic function parameter, number of items in List<ITsys*> for others
	};

	static bool operator==(const MatchBaseClassRecord& a, const MatchBaseClassRecord& b)
	{
		return a.parameterIndex == b.parameterIndex
			&& a.variadicIndex == b.variadicIndex
			&& a.start == b.start
			&& a.count == b.count
			;
	}

	bool CreateMbcr(const ParsingArguments& pa, Ptr<Type>& type, ITsys* value, MatchBaseClassRecord& mbcr, List<ITsys*>& mbcTsys)
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

		List<ITsys*> visited;
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

	void InferFunctionTypeInternal(const ParsingArguments& pa, FunctionType* functionType, List<ITsys*>& parameterAssignment, TemplateArgumentContext& taContext, SortedList<Symbol*>& freeTypeSymbols)
	{
		List<MatchBaseClassRecord> mbcs;
		List<ITsys*> mbcTsys;

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
			// TODO: not implemented
			throw 0;
		}

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