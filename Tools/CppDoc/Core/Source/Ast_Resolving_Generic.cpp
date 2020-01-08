#include "Ast_Resolving.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	GetTemplateSpecFromSymbol: Get TempalteSpec from a symbol if it is a generic declaration
	***********************************************************************/

	Ptr<TemplateSpec> GetTemplateSpecFromSymbol(Symbol* symbol)
	{
		if (symbol)
		{
			if (auto funcDecl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
			{
				return funcDecl->templateSpec;
			}
			else if (auto classDecl = symbol->GetAnyForwardDecl<ForwardClassDeclaration>())
			{
				return classDecl->templateSpec;
			}
			else if (auto typeAliasDecl = symbol->GetAnyForwardDecl<TypeAliasDeclaration>())
			{
				return typeAliasDecl->templateSpec;
			}
			else if (auto valueAliasDecl = symbol->GetAnyForwardDecl<ValueAliasDeclaration>())
			{
				return valueAliasDecl->templateSpec;
			}
		}
		return nullptr;
	}

	/***********************************************************************
	CreateGenericFunctionHeader: Calculate enough information to create a generic function type
	***********************************************************************/

	ITsys* GetTemplateArgumentKey(const TemplateSpec::Argument& argument, ITsysAlloc* tsys)
	{
		if (argument.argumentType == CppTemplateArgumentType::Type)
		{
			return EvaluateGenericArgumentSymbol(argument.argumentSymbol);
		}
		else
		{
			return tsys->DeclOf(argument.argumentSymbol);
		}
	}

	void CreateGenericFunctionHeader(const ParsingArguments& pa, Ptr<TemplateSpec> spec, TypeTsysList& params, TsysGenericFunction& genericFunction)
	{
		genericFunction.vtaArguments.Resize(spec->arguments.Count());
		genericFunction.acceptTypes.Resize(spec->arguments.Count());

		for (vint i = 0; i < spec->arguments.Count(); i++)
		{
			const auto& argument = spec->arguments[i];
			genericFunction.vtaArguments[i] = argument.ellipsis;
			genericFunction.acceptTypes[i] = (argument.argumentType == CppTemplateArgumentType::Type);
			params.Add(GetTemplateArgumentKey(argument, pa.tsys.Obj()));
		}
	}

	/***********************************************************************
	EnsureGenericTypeParameterAndArgumentMatched: Ensure values and types are passed in correct order
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

	/***********************************************************************
	GetArgumentCountRange: Calculate the minimum and maximum allowed argument count for a template, -1 means infinity
	***********************************************************************/

	void GetArgumentCountRange(ITsys* genericFunction, Ptr<TemplateSpec> spec, const TsysGenericFunction& genericFuncInfo, vint& minCount, vint& maxCount)
	{
		bool hasVta = From(genericFuncInfo.vtaArguments).Any([](bool x) { return x; });
		maxCount = hasVta ? -1 : genericFunction->GetParamCount();

		vint firstDefaultIndex = -1;
		if (spec)
		{
			for (vint i = 0; i < spec->arguments.Count(); i++)
			{
				const auto& argument = spec->arguments[i];
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

		if (hasVta)
		{
			// when there is a variant argument, the minimum count is the numbers of arguments before this variant argument
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

	/***********************************************************************
	CalculateGpa: Calculate how arguments are grouped and passed to template
	***********************************************************************/

	enum class GenericParameterAssignmentKind
	{
		DefaultValue,
		OneArgument,
		EmptyVta,
		MultipleVta,
		Any,
		Unfilled,
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

		static GenericParameterAssignment Unfilled()
		{
			return { GenericParameterAssignmentKind::Unfilled,-1,-1 };
		}
	};

	using GpaList = List<GenericParameterAssignment>;

	void CalculateGpa(
		GpaList& gpaMappings,							// list to store results
		const TsysGenericFunction& genericFuncInfo,		// generic type header
		vint inputArgumentCount,						// number of offered arguments
		SortedList<vint>& boundedAnys,					// boundedAnys[x] == i + offset means the i-th offered argument is any_t, and it means unknown variadic arguments, instead of an unknown type
		vint offset,									// offset to use with boundedAnys
		bool allowPartialApply							// true means it is legal to not offer enough amount of argunents
	)
	{
		// only fill arguments to the first variadic template argument
		// so if hasVta is true, only the last template argument to fill is variadic
		bool hasVta = false;
		vint parameterCount = genericFuncInfo.vtaArguments.Count();
		for (vint i = 0; i < genericFuncInfo.vtaArguments.Count(); i++)
		{
			if (genericFuncInfo.vtaArguments[i])
			{
				if (!allowPartialApply && i != parameterCount - 1)
				{
					throw NotConvertableException();
				}
				hasVta = true;
				parameterCount = i + 1;
				break;
			}
		}

		if (hasVta)
		{
			// if the generic declaration has variadic template arguments
			if (boundedAnys.Count() == 0)
			{
				// if there is no unknown variadic arguments
				for (vint i = 0; i < parameterCount; i++)
				{
					if (i < inputArgumentCount)
					{
						if (i == parameterCount - 1)
						{
							// pack all remaining offered arguments in this position for the variadic template argument
							gpaMappings.Add(GenericParameterAssignment::MultipleVta(i + offset, inputArgumentCount - i));
						}
						else
						{
							// use this offered argument in this position
							gpaMappings.Add(GenericParameterAssignment::OneArgument(i + offset));
						}
					}
					else
					{
						if (i == parameterCount - 1)
						{
							// use an empty pack in this position of the variadic template argument
							gpaMappings.Add(GenericParameterAssignment::EmptyVta());
						}
						else
						{
							// expect to use the default value in this position
							gpaMappings.Add(GenericParameterAssignment::DefaultValue());
						}
					}
				}
			}
			else
			{
				// if there are unknown variadic arguments
				vint headCount = boundedAnys[0] - offset;
				for (vint i = 0; i < parameterCount; i++)
				{
					if (i < headCount)
					{
						// if there is no unknown variadic arguments before this offered argument, use it in this position
						if (i == parameterCount - 1)
						{
							// pack all remaining offered arguments in this position for the variadic template argument
							// since there is at least one unknown variadic arguments to pack, use any here
							gpaMappings.Add(GenericParameterAssignment::Any());
						}
						else
						{
							// use this offered argument in this position
							gpaMappings.Add(GenericParameterAssignment::OneArgument(i + offset));
						}
					}
					else
					{
						// from the first unknown variadic arguments, fill any to all unfilled template arguments
						gpaMappings.Add(GenericParameterAssignment::Any());
					}
				}
			}
		}
		else
		{
			// if the generic declaration has no variadic template arguments
			if (boundedAnys.Count() == 0)
			{
				// if there is no unknown variadic arguments
				for (vint i = 0; i < parameterCount; i++)
				{
					if (i < inputArgumentCount)
					{
						// use this offered argument in this position
						gpaMappings.Add(GenericParameterAssignment::OneArgument(i + offset));
					}
					else
					{
						// expect to use the default value in this position
						gpaMappings.Add(GenericParameterAssignment::DefaultValue());
					}
				}
			}
			else
			{
				// if there are unknown variadic arguments
				vint headCount = boundedAnys[0] - offset;
				vint tailCount = inputArgumentCount - (boundedAnys[boundedAnys.Count() - 1] - offset) - 1;
				vint anyParameterCount = parameterCount - headCount - tailCount;

				for (vint i = 0; i < parameterCount; i++)
				{
					if (i < headCount)
					{
						// if there is no unknown variadic arguments before this offered argument, use it in this position
						gpaMappings.Add(GenericParameterAssignment::OneArgument(i + offset));
					}
					else if (i < headCount + anyParameterCount)
					{
						// this offered argument is a unknown variadic arguments list, or is between two list, use any in this position
						gpaMappings.Add(GenericParameterAssignment::Any());
					}
					else
					{
						// if there is no unknown variadic argument after this offered argument, use it in this position
						gpaMappings.Add(GenericParameterAssignment::OneArgument(inputArgumentCount - (parameterCount - i) + offset));
					}
				}
			}
		}
	}

	/***********************************************************************
	ResolveGenericParameters: Calculate generic parameter types by matching arguments to patterens
	***********************************************************************/

	void ResolveGenericParameters(
		const ParsingArguments& invokerPa,			// context
		TemplateArgumentContext& newTaContext,		// TAC to store type arguemnt to offered argument map, vta argument will be grouped to Init or Any
		ITsys* genericFunction,						// generic type header
		Array<ExprTsysItem>& argumentTypes,			// (index of unpacked)		offered argument (unpacked), starts from argumentTypes[offset]
		Array<bool>& isTypes,						// (index of packed)		isTypes[j + offset] == true means the j-th offered arguments (packed) is a type instead of a value
		Array<vint>& argSource,						// (unpacked -> packed)		argSource[i + offset] == j + offset means argumentTypes[i + offset] is from whole or part of the j-th packed offered argument
		SortedList<vint>& boundedAnys,				// (value of unpacked)		boundedAnys[x] == i + offset means argumentTypes[i + offset] is any_t, and it means unknown variadic arguments, instead of an unknown type
		vint offset,								// means first offset types are not template arguments or offered arguments, they stored other things
		bool allowPartialApply						// true means it is legal to not offer enough amount of argunents
	)
	{
		if (genericFunction->GetType() != TsysType::GenericFunction)
		{
			throw NotConvertableException();
		}

		const auto& genericFuncInfo = genericFunction->GetGenericFunction();
		auto spec = GetTemplateSpecFromSymbol(genericFuncInfo.declSymbol);

		vint minCount = -1;
		vint maxCount = -1;
		GetArgumentCountRange(genericFunction, spec, genericFuncInfo, minCount, maxCount);

		vint inputArgumentCount = argumentTypes.Count() - offset;
		if (boundedAnys.Count() == 0)
		{
			// if there is no unknown variadic arguments
			if (inputArgumentCount < minCount && !allowPartialApply)
			{
				// too few arguments
				throw NotConvertableException();
			}
			if (inputArgumentCount > maxCount && maxCount != -1)
			{
				// too many arguments
				throw NotConvertableException();
			}
		}
		else
		{
			// if there are unknown variadic arguments, only check if there are too many offered arguments
			if (maxCount != -1 && inputArgumentCount - boundedAnys.Count() > maxCount)
			{
				throw NotConvertableException();
			}
		}

		// calculate how to assign offered arguments to template arguments
		// gpaMappings will contains decisions for every template arguments
		// if there are not enough offered arguments, only the first few templates are assigned decisions
		GpaList gpaMappings;
		CalculateGpa(gpaMappings, genericFuncInfo, inputArgumentCount, boundedAnys, offset, allowPartialApply);

		auto pa = invokerPa.AdjustForDecl(genericFuncInfo.declSymbol).AppendSingleLevelArgs(newTaContext);
		for (vint i = 0; i < genericFunction->GetParamCount(); i++)
		{
			auto gpa = gpaMappings[i];
			auto pattern = genericFunction->GetParam(i);
			bool acceptType = genericFuncInfo.acceptTypes[i];

			switch (gpa.kind)
			{
			case GenericParameterAssignmentKind::DefaultValue:
				{
					// if a default value is expected to fill this template argument
					if (acceptType)
					{
						if (spec->arguments[i].argumentType == CppTemplateArgumentType::Value)
						{
							throw NotConvertableException();
						}
						TypeTsysList argTypes;
						TypeToTsysNoVta(pa.WithScope(genericFuncInfo.declSymbol), spec->arguments[i].type, argTypes);

						for (vint j = 0; j < argTypes.Count(); j++)
						{
							newTaContext.arguments.Add(pattern, argTypes[j]);
						}
					}
					else
					{
						if (spec->arguments[i].argumentType != CppTemplateArgumentType::Value)
						{
							throw NotConvertableException();
						}
						newTaContext.arguments.Add(pattern, nullptr);
					}
				}
				break;
			case GenericParameterAssignmentKind::OneArgument:
				{
					// if an offered argument is to fill this template argument
					if (acceptType != isTypes[argSource[gpa.index]])
					{
						throw NotConvertableException();
					}

					if (acceptType)
					{
						auto item = argumentTypes[gpa.index];
						EnsureGenericTypeParameterAndArgumentMatched(pattern, item.tsys);
						newTaContext.arguments.Add(pattern, item.tsys);
					}
					else
					{
						newTaContext.arguments.Add(pattern, nullptr);
					}
				}
				break;
			case GenericParameterAssignmentKind::EmptyVta:
				{
					// if an empty pack of offered arguments is to fill this variadic template argument
					Array<ExprTsysItem> items;
					auto init = pa.tsys->InitOf(items);
					newTaContext.arguments.Add(pattern, init);
				}
				break;
			case GenericParameterAssignmentKind::MultipleVta:
				{
					// if a pack of offered arguments is to fill this variadic template argument
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
					newTaContext.arguments.Add(pattern, init);
				}
				break;
			case GenericParameterAssignmentKind::Any:
				{
					// if any is to fill this (maybe variadic) template argument
					newTaContext.arguments.Add(pattern, pa.tsys->Any());
				}
				break;
			case GenericParameterAssignmentKind::Unfilled:
				{
					// if this template argument is not filled
					// no default or empty for unfilled arguments, check them here
					throw NotConvertableException();
				}
				break;
			}
		}
	}
}