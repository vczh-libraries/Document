#include "Ast_Resolving.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	CreateGenericFunctionHeader: Calculate enough information to create a generic function type
	***********************************************************************/

	void CreateGenericFunctionHeader(const ParsingArguments& pa, Ptr<TemplateSpec> spec, TypeTsysList& params, TsysGenericFunction& genericFunction)
	{
		genericFunction.isLastParameterVta = false;
		genericFunction.acceptTypes.Resize(spec->arguments.Count());

		for (vint i = 0; i < spec->arguments.Count(); i++)
		{
			auto argument = spec->arguments[i];
			if ((genericFunction.acceptTypes[i] = (argument.argumentType == CppTemplateArgumentType::Type)))
			{
				params.Add(argument.argumentSymbol->evaluation.Get()[0]);
			}
			else
			{
				params.Add(pa.tsys->DeclOf(argument.argumentSymbol));
			}

			if (argument.ellipsis)
			{
				if (!genericFunction.isLastParameterVta)
				{
					genericFunction.isLastParameterVta = true;
				}
				else
				{
					throw NotConvertableException();
				}
			}
		}
	}

	/***********************************************************************
	ResolveGenericParameters: Calculate generic parameter types by matching arguments to patterens
	***********************************************************************/

	void EnsureGenericFunctionParameterAndArgumentMatched(ITsys* parameter, ITsys* argument);

	void EnsureGenericTypeParameterAndArgumentMatched(ITsys* parameter, ITsys* argument)
	{
		switch (parameter->GetType())
		{
		case TsysType::GenericArg:
			if (argument->GetType() == TsysType::GenericFunction)
			{
				throw NotConvertableException();
			}
			break;
		case TsysType::GenericFunction:
			if (argument->GetType() != TsysType::GenericFunction)
			{
				throw NotConvertableException();
			}
			EnsureGenericFunctionParameterAndArgumentMatched(parameter, argument);
			break;
		default:
			// until class specialization begins to develop, this should always not happen
			throw NotConvertableException();
		}
	}

	void EnsureGenericFunctionParameterAndArgumentMatched(ITsys* parameter, ITsys* argument)
	{
		if (parameter->GetParamCount() != argument->GetParamCount())
		{
			throw NotConvertableException();
		}
		for (vint i = 0; i < parameter->GetParamCount(); i++)
		{
			auto nestedParameter = parameter->GetParam(i);
			auto nestedArgument = argument->GetParam(i);

			bool acceptType = parameter->GetGenericFunction().acceptTypes[i];
			bool isType = argument->GetGenericFunction().acceptTypes[i];
			if (acceptType != isType)
			{
				throw NotConvertableException();
			}

			if (acceptType)
			{
				EnsureGenericTypeParameterAndArgumentMatched(nestedParameter, nestedArgument);
			}
		}
	}

	Ptr<TemplateSpec> GetTemplateSpec(const TsysGenericFunction& genericFuncInfo)
	{
		if (genericFuncInfo.declSymbol)
		{
			if (auto typeAliasDecl = genericFuncInfo.declSymbol->GetAnyForwardDecl<TypeAliasDeclaration>())
			{
				return typeAliasDecl->templateSpec;
			}
			else if (auto valueAliasDecl = genericFuncInfo.declSymbol->GetAnyForwardDecl<ValueAliasDeclaration>())
			{
				return valueAliasDecl->templateSpec;
			}
		}
		return nullptr;
	}

	void GetArgumentCountRange(ITsys* genericFunction, Ptr<TemplateSpec> spec, const TsysGenericFunction& genericFuncInfo, vint& minCount, vint& maxCount)
	{
		maxCount = genericFuncInfo.isLastParameterVta ? -1 : genericFunction->GetParamCount();

		vint firstDefaultIndex = -1;
		if (spec)
		{
			for (vint i = 0; i < spec->arguments.Count(); i++)
			{
				auto argument = spec->arguments[i];
				if (argument.ellipsis)
				{
					firstDefaultIndex = i;
					goto FINISH_FINDING_DEFAULT;
				}
				else
				{
					switch (argument.argumentType)
					{
					case CppTemplateArgumentType::HighLevelType:
					case CppTemplateArgumentType::Type:
						if (argument.type)
						{
							firstDefaultIndex = i;
							goto FINISH_FINDING_DEFAULT;
						}
						break;
					case CppTemplateArgumentType::Value:
						if (argument.expr)
						{
							firstDefaultIndex = i;
							goto FINISH_FINDING_DEFAULT;
						}
						break;
					}
				}
			}
		}
	FINISH_FINDING_DEFAULT:

		if (genericFuncInfo.isLastParameterVta)
		{
			// last_argument.ellipsis == -1, so firstDefaultIndex will not be -1
			minCount = firstDefaultIndex;
			maxCount = -1;
		}
		else
		{
			if (firstDefaultIndex == -1)
			{
				minCount = genericFunction->GetParamCount();
			}
			else
			{
				minCount = firstDefaultIndex;
			}
			maxCount = genericFunction->GetParamCount();
		}
	}

	enum class GenericParameterAssignmentKind
	{
		DefaultValue,
		OneArgument,
		EmptyVta,
		MultipleVta,
		Any,
	};

	struct GenericParameterAssignment
	{
		GenericParameterAssignmentKind		kind;
		vint								index;
		vint								count;

		GenericParameterAssignment() = default;
		GenericParameterAssignment(GenericParameterAssignmentKind _kind, vint _index, vint _count)
			:kind(_kind)
			, index(_index)
			, count(_count)
		{
		}

		static GenericParameterAssignment DefaultValue()
		{
			return { GenericParameterAssignmentKind::DefaultValue,-1,-1 };
		}

		static GenericParameterAssignment OneArgument(vint index)
		{
			return { GenericParameterAssignmentKind::OneArgument,index,-1 };
		}

		static GenericParameterAssignment EmptyVta()
		{
			return { GenericParameterAssignmentKind::EmptyVta,-1,-1 };
		}

		static GenericParameterAssignment MultipleVta(vint index, vint count)
		{
			return { GenericParameterAssignmentKind::MultipleVta,index,count };
		}

		static GenericParameterAssignment Any()
		{
			return { GenericParameterAssignmentKind::Any,-1,-1 };
		}
	};

	using GpaList = List<GenericParameterAssignment>;

	void CalculateGpa(GpaList& gpaMappings, ITsys* genericFunction, const TsysGenericFunction& genericFuncInfo, vint inputArgumentCount, SortedList<vint>& boundedAnys, vint offset)
	{
		vint parameterCount = genericFunction->GetParamCount();
		if (genericFuncInfo.isLastParameterVta)
		{
			if (boundedAnys.Count() == 0)
			{
				for (vint i = 0; i < parameterCount; i++)
				{
					if (i < inputArgumentCount)
					{
						if (i == parameterCount - 1)
						{
							gpaMappings.Add(GenericParameterAssignment::MultipleVta(i + offset, inputArgumentCount - i));
						}
						else
						{
							gpaMappings.Add(GenericParameterAssignment::OneArgument(i + offset));
						}
					}
					else
					{
						if (i == parameterCount - 1)
						{
							gpaMappings.Add(GenericParameterAssignment::EmptyVta());
						}
						else
						{
							gpaMappings.Add(GenericParameterAssignment::DefaultValue());
						}
					}
				}
			}
			else
			{
				vint headCount = boundedAnys[0] - offset;
				for (vint i = 0; i < parameterCount; i++)
				{
					if (i < headCount)
					{
						if (i == parameterCount - 1)
						{
							gpaMappings.Add(GenericParameterAssignment::Any());
						}
						else
						{
							gpaMappings.Add(GenericParameterAssignment::OneArgument(i + offset));
						}
					}
					else
					{
						gpaMappings.Add(GenericParameterAssignment::Any());
					}
				}
			}
		}
		else
		{
			if (boundedAnys.Count() == 0)
			{
				for (vint i = 0; i < parameterCount; i++)
				{
					if (i < inputArgumentCount)
					{
						gpaMappings.Add(GenericParameterAssignment::OneArgument(i + offset));
					}
					else
					{
						gpaMappings.Add(GenericParameterAssignment::DefaultValue());
					}
				}
			}
			else
			{
				vint headCount = boundedAnys[0] - offset;
				vint tailCount = inputArgumentCount - (boundedAnys[boundedAnys.Count() - 1] - offset) - 1;
				vint anyParameterCount = parameterCount - headCount - tailCount;

				for (vint i = 0; i < parameterCount; i++)
				{
					if (i < headCount)
					{
						gpaMappings.Add(GenericParameterAssignment::OneArgument(i + offset));
					}
					else if (i < headCount + anyParameterCount)
					{
						gpaMappings.Add(GenericParameterAssignment::Any());
					}
					else
					{
						gpaMappings.Add(GenericParameterAssignment::OneArgument(inputArgumentCount - (parameterCount - i) + offset));
					}
				}
			}
		}
	}

	void ResolveGenericParameters(const ParsingArguments& pa, ITsys* genericFunction, Array<ExprTsysItem>& argumentTypes, Array<bool>& isTypes, Array<vint>& argSource, SortedList<vint>& boundedAnys, vint offset, GenericArgContext* newGaContext)
	{
		if (genericFunction->GetType() != TsysType::GenericFunction)
		{
			throw NotConvertableException();
		}

		const auto& genericFuncInfo = genericFunction->GetGenericFunction();
		auto spec = GetTemplateSpec(genericFuncInfo);

		vint minCount = -1;
		vint maxCount = -1;
		GetArgumentCountRange(genericFunction, spec, genericFuncInfo, minCount, maxCount);

		vint inputArgumentCount = argumentTypes.Count() - offset;
		if (boundedAnys.Count() == 0)
		{
			if (inputArgumentCount < minCount)
			{
				throw NotConvertableException();
			}
			if (maxCount != -1 && inputArgumentCount > maxCount)
			{
				throw NotConvertableException();
			}
		}
		else
		{
			if (maxCount != -1 && inputArgumentCount - boundedAnys.Count() > maxCount)
			{
				throw NotConvertableException();
			}
		}

		GpaList gpaMappings;
		CalculateGpa(gpaMappings, genericFunction, genericFuncInfo, inputArgumentCount, boundedAnys, offset);

		for (vint i = 0; i < genericFunction->GetParamCount(); i++)
		{
			auto gpa = gpaMappings[i];
			auto pattern = genericFunction->GetParam(i);
			bool acceptType = genericFuncInfo.acceptTypes[i];

			switch (gpa.kind)
			{
			case GenericParameterAssignmentKind::DefaultValue:
				{
					if (acceptType)
					{
						if (spec->arguments[i].argumentType == CppTemplateArgumentType::Value)
						{
							throw NotConvertableException();
						}
						TypeTsysList argTypes;
						TypeToTsysNoVta(pa.WithContext(genericFuncInfo.declSymbol), spec->arguments[i].type, argTypes, newGaContext);

						for (vint j = 0; j < argTypes.Count(); j++)
						{
							newGaContext->arguments.Add(pattern, argTypes[j]);
						}
					}
					else
					{
						if (spec->arguments[i].argumentType != CppTemplateArgumentType::Value)
						{
							throw NotConvertableException();
						}
						newGaContext->arguments.Add(pattern, nullptr);
					}
				}
				break;
			case GenericParameterAssignmentKind::OneArgument:
				{
					if (acceptType != isTypes[argSource[gpa.index]])
					{
						throw NotConvertableException();
					}

					if (acceptType)
					{
						auto item = argumentTypes[gpa.index];
						EnsureGenericTypeParameterAndArgumentMatched(pattern, item.tsys);
						newGaContext->arguments.Add(pattern, item.tsys);
					}
					else
					{
						newGaContext->arguments.Add(pattern, nullptr);
					}
				}
				break;
			case GenericParameterAssignmentKind::EmptyVta:
				{
					Array<ExprTsysItem> items;
					auto init = pa.tsys->InitOf(items);
					newGaContext->arguments.Add(pattern, init);
				}
				break;
			case GenericParameterAssignmentKind::MultipleVta:
				{
					for (vint j = 0; j < gpa.count; j++)
					{
						if (acceptType != isTypes[argSource[gpa.index + j]])
						{
							throw NotConvertableException();
						}
					}

					Array<ExprTsysItem> items(gpa.count);

					for (vint j = 0; j < gpa.count; j++)
					{
						if (acceptType)
						{
							auto item = argumentTypes[gpa.index + j];
							EnsureGenericTypeParameterAndArgumentMatched(pattern, item.tsys);
							items[j] = item;
						}
						else
						{
							items[j] = { nullptr,ExprTsysType::PRValue,nullptr };
						}
					}
					auto init = pa.tsys->InitOf(items);
					newGaContext->arguments.Add(pattern, init);
				}
				break;
			case GenericParameterAssignmentKind::Any:
				{
					newGaContext->arguments.Add(pattern, pa.tsys->Any());
				}
				break;
			}
		}
	}
}