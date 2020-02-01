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

	void CreateGenericFunctionHeader(const ParsingArguments& pa, Symbol* declSymbol, ITsys* parentDeclType, Ptr<TemplateSpec> spec, TypeTsysList& params, TsysGenericFunction& genericFunction)
	{
		if (!declSymbol) throw TypeCheckerException();
		if (!spec) throw TypeCheckerException();

		genericFunction.declSymbol = declSymbol;
		genericFunction.parentDeclType = parentDeclType;
		genericFunction.spec = spec;
		for (vint i = 0; i < spec->arguments.Count(); i++)
		{
			const auto& argument = spec->arguments[i];
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
				throw TypeCheckerException();
			}
			break;
		case TsysType::GenericFunction:
			if (argument->GetType() != TsysType::GenericFunction)
			{
				throw TypeCheckerException();
			}
			EnsureGenericFunctionParameterAndArgumentMatched(parameter, argument);
			break;
		default:
			// until class specialization begins to develop, this should always not happen
			throw TypeCheckerException();
		}
	}

	void EnsureGenericFunctionParameterAndArgumentMatched(ITsys* parameter, ITsys* argument)
	{
		if (parameter->GetParamCount() != argument->GetParamCount())
		{
			throw TypeCheckerException();
		}

		auto& pGF = parameter->GetGenericFunction();
		auto& aGF = argument->GetGenericFunction();
		for (vint i = 0; i < parameter->GetParamCount(); i++)
		{
			auto nestedParameter = parameter->GetParam(i);
			auto nestedArgument = argument->GetParam(i);

			auto pT = pGF.spec->arguments[i].argumentType;
			auto aT = aGF.spec->arguments[i].argumentType;
			if (pT != aT)
			{
				throw TypeCheckerException();
			}

			if (pT != CppTemplateArgumentType::Value)
			{
				EnsureGenericTypeParameterAndArgumentMatched(nestedParameter, nestedArgument);
			}
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

	template<typename TIsVta, typename THasDefault>
	void CalculateGpa(
		GpaList& gpaMappings,							// list to store results
		vint inputArgumentCount,						// number of offered arguments
		SortedList<vint>& boundedAnys,					// boundedAnys[x] == i + offset means the i-th offered argument is any_t, and it means unknown variadic arguments, instead of an unknown type
		vint offset,									// offset to use with boundedAnys
		bool allowPartialApply,							// true means it is legal to not offer enough amount of arguments
		vint templateArgumentCount,						// number of template argument
		TIsVta&& argIsVat,								// test if a template argument is variadic
		THasDefault&& argHasDefault						// test if a template argument has a default value
	)
	{
		// only fill arguments to the first variadic template argument
		vint firstDefault = -1;
		vint firstVta = -1;

		for (vint i = 0; i < templateArgumentCount; i++)
		{
			if (argIsVat(i))
			{
				if (!allowPartialApply && i != templateArgumentCount - 1)
				{
					throw TypeCheckerException();
				}
				firstVta = i;
				break;
			}

			if (argHasDefault(i))
			{
				if (firstDefault == -1)
				{
					firstDefault = i;
				}
			}
			else
			{
				if (firstDefault != -1)
				{
					throw TypeCheckerException();
				}
			}
		}

		// check if there are too much offered arguments
		vint parametersToFill = firstVta == -1 ? templateArgumentCount : firstVta + 1;
		// check if there are too many offered arguments
		if (firstVta == -1 && inputArgumentCount - boundedAnys.Count() > parametersToFill)
		{
			throw TypeCheckerException();
		}

		if (firstVta == -1)
		{
			// if the generic declaration has no variadic template arguments
			if (boundedAnys.Count() == 0)
			{
				// if there is no unknown variadic arguments
				for (vint i = 0; i < parametersToFill; i++)
				{
					if (i < inputArgumentCount)
					{
						// if there is an offered argument, use this offered argument in this position
						gpaMappings.Add(GenericParameterAssignment::OneArgument(i + offset));
					}
					else if (firstDefault != -1 && firstDefault <= i)
					{
						// if there is a default value, expect to use the default value in this position
						gpaMappings.Add(GenericParameterAssignment::DefaultValue());
					}
					else if (allowPartialApply)
					{
						// if missing offered arguments are allowed
						gpaMappings.Add(GenericParameterAssignment::Unfilled());
					}
					else
					{
						// missing offered arguments
						throw TypeCheckerException();
					}
				}
			}
			else
			{
				// if there are unknown variadic arguments
				vint headCount = boundedAnys[0] - offset;
				vint tailCount = inputArgumentCount - (boundedAnys[boundedAnys.Count() - 1] - offset) - 1;
				vint anyParameterCount = parametersToFill - headCount - tailCount;

				for (vint i = 0; i < parametersToFill; i++)
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
						gpaMappings.Add(GenericParameterAssignment::OneArgument(inputArgumentCount - (parametersToFill - i) + offset));
					}
				}
			}
		}
		else
		{
			vint minOffered = firstDefault == -1
				? firstVta
				: (firstDefault < firstVta ? firstDefault : firstVta)
				;
			// if the generic declaration has variadic template arguments
			if (boundedAnys.Count() == 0)
			{
				// if there is no unknown variadic arguments
				for (vint i = 0; i < parametersToFill; i++)
				{
					if (i < inputArgumentCount)
					{
						// if there is an offered argument
						if (i == firstVta)
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
						// if there is no offered argument
						if (i == firstVta)
						{
							if (inputArgumentCount >= minOffered)
							{
								// if there is no unfilled template arguments before this variadic template argument
								// use an empty pack in this position of the variadic template argument
								gpaMappings.Add(GenericParameterAssignment::EmptyVta());
							}
							else
							{
								// if there are unfilled template arguments before this variadic template argument
								// it misses an offered argument
								// if missing offered arguments are not allowed, it crashes before here
								gpaMappings.Add(GenericParameterAssignment::Unfilled());
							}
						}
						else if (firstDefault != -1 && firstDefault <= i)
						{
							// if there is a default value, expect to use the default value in this position
							gpaMappings.Add(GenericParameterAssignment::DefaultValue());
						}
						else if (allowPartialApply)
						{
							// if missing offered arguments are allowed
							gpaMappings.Add(GenericParameterAssignment::Unfilled());
						}
						else
						{
							// missing offered arguments
							throw TypeCheckerException();
						}
					}
				}
			}
			else
			{
				// if there are unknown variadic arguments
				vint headCount = boundedAnys[0] - offset;
				for (vint i = 0; i < parametersToFill; i++)
				{
					if (i < headCount)
					{
						// if there is no unknown variadic arguments before this offered argument, use it in this position
						if (i == firstVta)
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

		if (parametersToFill < templateArgumentCount)
		{
			// if there are multiple variadic template arguments, they are all unfilled
			for (vint i = parametersToFill; i < templateArgumentCount; i++)
			{
				if (!argIsVat(i))
				{
					throw TypeCheckerException();
				}
				gpaMappings.Add(GenericParameterAssignment::Unfilled());
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
		Array<bool>& isTypes,						// (index of packed)		( isTypes[j + offset] == true )			means the j-th offered arguments (packed) is a type instead of a value
		Array<vint>& argSource,						// (unpacked -> packed)		( argSource[i + offset] == j + offset )	means argumentTypes[i + offset] is from whole or part of the j-th packed offered argument
		SortedList<vint>& boundedAnys,				// (value of unpacked)		( boundedAnys[x] == i + offset )		means argumentTypes[i + offset] is any_t, and it means unknown variadic arguments, instead of an unknown type
		vint offset,								// means first offset types are not template arguments or offered arguments, they stored other things
		bool allowPartialApply,						// true means it is legal to not offer enough amount of arguments
		vint& partialAppliedArguments				// the number of applied template arguments, -1 if all arguments are filled
	)
	{
		if (genericFunction->GetType() != TsysType::GenericFunction)
		{
			throw TypeCheckerException();
		}

		const auto& genericFuncInfo = genericFunction->GetGenericFunction();
		auto spec = genericFuncInfo.spec;
		vint inputArgumentCount = argumentTypes.Count() - offset;

		// calculate how to assign offered arguments to template arguments
		// gpaMappings will contains decisions for every template arguments
		// if there are not enough offered arguments, only the first few templates are assigned decisions
		GpaList gpaMappings;
		CalculateGpa(gpaMappings, inputArgumentCount, boundedAnys, offset, allowPartialApply, spec->arguments.Count(),
			[&spec](vint index) { return spec->arguments[index].ellipsis; },
			[&spec](vint index) { auto argument = spec->arguments[index]; return argument.argumentType == CppTemplateArgumentType::Value ? argument.expr : argument.type; }
			);

		auto pa = invokerPa.AdjustForDecl(genericFuncInfo.declSymbol).AppendSingleLevelArgs(newTaContext);
		for (vint i = 0; i < genericFunction->GetParamCount(); i++)
		{
			auto gpa = gpaMappings[i];
			auto pattern = genericFunction->GetParam(i);
			bool acceptType = spec->arguments[i].argumentType != CppTemplateArgumentType::Value;

			switch (gpa.kind)
			{
			case GenericParameterAssignmentKind::DefaultValue:
				{
					// if a default value is expected to fill this template argument
					if (acceptType)
					{
						if (spec->arguments[i].argumentType == CppTemplateArgumentType::Value)
						{
							throw TypeCheckerException();
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
							throw TypeCheckerException();
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
						throw TypeCheckerException();
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
							throw TypeCheckerException();
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
					// fill them will template argument themselves
					partialAppliedArguments = i;
					for (vint j = i; j < genericFunction->GetParamCount(); j++)
					{
						auto unappliedPattern = genericFunction->GetParam(j);
						auto unappliedValue = spec->arguments[j].ellipsis ? pa.tsys->Any() : acceptType ? unappliedPattern : nullptr;
						newTaContext.arguments.Add(unappliedPattern, unappliedValue);
					}
					return;
				}
				break;
			}
		}
		partialAppliedArguments = -1;
	}

	/***********************************************************************
	ResolveFunctionParameters: Calculate function parameter types by matching arguments to patterens
	***********************************************************************/

	void AssignParameterAssignment(
		const ParsingArguments& invokerPa,
		vint parameterCount,
		TypeTsysList& parameterAssignment,
		GpaList& gpaMappings,
		Array<ExprTsysItem>& argumentTypes
	)
	{
		for (vint i = 0; i < parameterCount; i++)
		{
			auto gpa = gpaMappings[i];

			switch (gpa.kind)
			{
			case GenericParameterAssignmentKind::DefaultValue:
				{
					// if a default value is expected to fill this template argument
					parameterAssignment.Add(nullptr);
				}
				break;
			case GenericParameterAssignmentKind::OneArgument:
				{
					// if an offered argument is to fill this template argument
					parameterAssignment.Add(ApplyExprTsysType(argumentTypes[gpa.index].tsys, argumentTypes[gpa.index].type));
				}
				break;
			case GenericParameterAssignmentKind::EmptyVta:
				{
					// if an empty pack of offered arguments is to fill this variadic template argument
					Array<ExprTsysItem> items;
					auto init = invokerPa.tsys->InitOf(items);
					parameterAssignment.Add(init);
				}
				break;
			case GenericParameterAssignmentKind::MultipleVta:
				{
					// if a pack of offered arguments is to fill this variadic template argument
					Array<ExprTsysItem> items(gpa.count);

					for (vint j = 0; j < gpa.count; j++)
					{
						items[j] = argumentTypes[gpa.index + j];
					}
					auto init = invokerPa.tsys->InitOf(items);
					parameterAssignment.Add(init);
				}
				break;
			case GenericParameterAssignmentKind::Any:
				{
					// if any is to fill this (maybe variadic) template argument
					parameterAssignment.Add(invokerPa.tsys->Any());
				}
				break;
			default:
				// missing arguments are not allowed
				throw TypeCheckerException();
				break;
			}
		}
	}

	void ResolveFunctionParameters(
		const ParsingArguments& invokerPa,			// context
		TypeTsysList& parameterAssignment,			// store function argument to offered argument map, nullptr indicates the default value is applied
		FunctionType* functionType,					// argument information
		Array<ExprTsysItem>& argumentTypes,			// (index of unpacked)		offered argument (unpacked)
		SortedList<vint>& boundedAnys				// (value of unpacked)		for each offered argument that is any_t, and it means unknown variadic arguments, instead of an unknown type
	)
	{
		// calculate how to assign offered arguments to function arguments
		// gpaMappings will contains decisions for every template arguments
		vint functionParameterCount = functionType->parameters.Count();
		GpaList gpaMappings;
		CalculateGpa(gpaMappings, argumentTypes.Count(), boundedAnys, 0, false, functionParameterCount + (functionType->ellipsis ? 1 : 0),
			[functionType](vint index) { return index == functionType->parameters.Count() ? functionType->ellipsis : functionType->parameters[index].isVariadic; },
			[functionType](vint index) { return index == functionType->parameters.Count() ? false : (bool)functionType->parameters[index].item->initializer; }
		);
		AssignParameterAssignment(invokerPa, functionParameterCount, parameterAssignment, gpaMappings, argumentTypes);
	}

	/***********************************************************************
	ResolveGenericTypeParameters: Calculate generic parameter types by matching arguments to patterens
	***********************************************************************/

	void ResolveGenericTypeParameters(
		const ParsingArguments& invokerPa,			// context
		TypeTsysList& parameterAssignment,			// store function argument to offered argument map, nullptr indicates the default value is applied
		GenericType* genericType,					// argument information
		Array<ExprTsysItem>& argumentTypes,			// (index of unpacked)		offered argument (unpacked)
		SortedList<vint>& boundedAnys				// (value of unpacked)		for each offered argument that is any_t, and it means unknown variadic arguments, instead of an unknown type
	)
	{
		// calculate how to assign offered arguments to generic type arguments
		// gpaMappings will contains decisions for every template arguments
		vint genericParameterCount = genericType->arguments.Count();
		GpaList gpaMappings;
		// set allowPartialApply to true because, arguments of genericType could be incomplete, but argumentTypes are always complete because it comes from a ITsys*
		CalculateGpa(gpaMappings, argumentTypes.Count(), boundedAnys, 0, true, genericParameterCount + 1,
			[genericType](vint index) { return index == genericType->arguments.Count() ? true : genericType->arguments[index].isVariadic; },
			[](vint index) { return false; }
		);
		AssignParameterAssignment(invokerPa, genericParameterCount, parameterAssignment, gpaMappings, argumentTypes);
	}
}