#include "Ast_Resolving.h"
#include "Ast_Resolving_IFT.h"

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
	GetTemplateArgumentKey: Get an ITsys* for TsysGenericFunction keys
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

	ITsys* GetTemplateArgumentKey(Symbol* argumentSymbol, ITsysAlloc* tsys)
	{
		switch (argumentSymbol->kind)
		{
		case symbol_component::SymbolKind::GenericTypeArgument:
			return EvaluateGenericArgumentSymbol(argumentSymbol);
		case symbol_component::SymbolKind::GenericValueArgument:
			return tsys->DeclOf(argumentSymbol);
		default:
			throw TypeCheckerException();
		}
	}

	/***********************************************************************
	CreateGenericFunctionHeader: Calculate enough information to create a generic function type
	***********************************************************************/

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
		Array<vint>& knownPackSizes,					// pack size of all template arguments, -1 for unknown, empty array to ignore
		TIsVta&& argIsVat,								// test if a template argument is variadic
		THasDefault&& argHasDefault						// test if a template argument has a default value
	)
	{
		// both template arguments and offered arguments could have unknown pack size
		// for template argument, it satisfies knownPackSizes[argumentIndex] == -1
		// for offered argument, it satisfies boundedAnys[x] == argumentIndex

		// fill knownPackSizes if empty
		if (knownPackSizes.Count() == 0)
		{
			knownPackSizes.Resize(templateArgumentCount);
			for (vint i = 0; i < templateArgumentCount; i++)
			{
				knownPackSizes[i] = argIsVat(i) ? -1 : 1;
			}
		}

		// first offered argument with unknown pack size
		vint firstBoundedAny = boundedAnys.Count() > 0 ? boundedAnys[0] - offset : -1;

		// when given up, fill the rest of arguments with any_t
		bool givenUp = false;

		vint readingInput = 0;
		bool seenVat = false;

		for (vint i = 0; i < templateArgumentCount; i++)
		{
			if (givenUp)
			{
				gpaMappings.Add(GenericParameterAssignment::Any());
			}
			else
			{
				if (seenVat)
				{
					// if there are arguments after the first variadic template argument with unknown pack size
					if (allowPartialApply)
					{
						// since all offered arguments has been used, unfill them for both variadic or non-variadic template argument
						gpaMappings.Add(GenericParameterAssignment::Unfilled());
					}
					else
					{
						// not enough offered arguments
						throw TypeCheckerException();
					}
				}
				else if (argIsVat(i))
				{
					// for variadic template argument, there is no default values
					vint packSize = knownPackSizes[i];
					if (packSize == -1)
					{
						// if the pack size is unknown
						seenVat = true;
						if (firstBoundedAny != -1)
						{
							// if there is any offered argument with unknown pack size, give up
							givenUp = true;
							i--;
							readingInput = inputArgumentCount;
							continue;
						}
						else
						{
							// give all remaining offered arguments to it
							if (readingInput == inputArgumentCount)
							{
								gpaMappings.Add(GenericParameterAssignment::EmptyVta());
							}
							else
							{
								gpaMappings.Add(GenericParameterAssignment::MultipleVta(readingInput + offset, inputArgumentCount - readingInput));
								readingInput = inputArgumentCount;
							}
						}
					}
					else
					{
						// if the pack size is known
						if (firstBoundedAny != -1 && readingInput + packSize > firstBoundedAny)
						{
							// an offered argument with unknown pack size has been read, give up
							givenUp = true;
							i--;
							readingInput = inputArgumentCount;
							continue;
						}
						else if (readingInput + packSize <= inputArgumentCount)
						{
							// if there are still enough offered arguments to apply
							if (packSize == 0)
							{
								gpaMappings.Add(GenericParameterAssignment::EmptyVta());
							}
							else
							{
								gpaMappings.Add(GenericParameterAssignment::MultipleVta(readingInput + offset, packSize));
								readingInput += packSize;
							}
						}
						else if (allowPartialApply)
						{
							// if partial applying is allowed
							gpaMappings.Add(GenericParameterAssignment::Unfilled());
						}
						else
						{
							// not enough offered arguments
							throw TypeCheckerException();
						}
					}
				}
				else
				{
					// for non-variadic template argument
					if (firstBoundedAny != -1 && readingInput >= firstBoundedAny)
					{
						// an offered argument with unknown pack size has been read, give up
						givenUp = true;
						i--;
						readingInput = inputArgumentCount;
						continue;
					}
					else if (readingInput < inputArgumentCount)
					{
						// if there is still an offered argument to apply
						gpaMappings.Add(GenericParameterAssignment::OneArgument(readingInput + offset));
						readingInput++;
					}
					else if (argHasDefault(i))
					{
						// if there is a default value
						gpaMappings.Add(GenericParameterAssignment::DefaultValue());
					}
					else if (allowPartialApply)
					{
						// if partial applying is allowed
						gpaMappings.Add(GenericParameterAssignment::Unfilled());
					}
					else
					{
						// not enough offered arguments
						throw TypeCheckerException();
					}
				}
			}
		}

		if (!givenUp && readingInput < inputArgumentCount)
		{
			// check if all the rest are offered arguments with unknown pack size
			// since all these offered arguments has not been read, just check the count
			vint remainingCount = inputArgumentCount - readingInput;
			if (boundedAnys.Count() != remainingCount)
			{
				// too many offered arguments
				throw TypeCheckerException();
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
		Array<vint> knownPackSizes;
		CalculateGpa(gpaMappings, inputArgumentCount, boundedAnys, offset, allowPartialApply, spec->arguments.Count(), knownPackSizes,
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
	ResolveGenericTypeParameters: Calculate generic parameter types by matching arguments to patterens
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

	vint CalculateParameterPackSize(
		const ParsingArguments& invokerPa,
		const TemplateArgumentContext& knownArguments,
		const SortedList<Type*>& involvedTypes,
		const SortedList<Expr*>& involvedExprs
	)
	{
		vint packSize = -1;
		bool conflicted = false;
		CollectInvolvedVariadicArguments(invokerPa, involvedTypes, involvedExprs, [&invokerPa, &packSize, &conflicted, &knownArguments](Symbol*, ITsys* pattern)
		{
			vint index = knownArguments.arguments.Keys().IndexOf(pattern);
			ITsys* pack = nullptr;
			if (index != -1)
			{
				pack = knownArguments.arguments.Values()[index];
			}
			else
			{
				invokerPa.TryGetReplacedGenericArg(pattern, pack);
			}

			if (pack && pack->GetType() == TsysType::Init)
			{
				vint newPackSize = pack->GetParamCount();
				if (packSize == -1)
				{
					packSize = newPackSize;
				}
				else if (packSize != newPackSize)
				{
					conflicted = true;
				}
			}
		});

		if (conflicted)
		{
			// all assigned variadic template arguments that are used in a function parameter should have the same pack size
			throw TypeCheckerException();
		}
		else
		{
			return packSize;
		}
	}

	// adjust pa so that taContext of parentDeclType is used accordingly
	// this adjustment is only valid for following functions
	// because only tsys and TryGetReplacedGenericArg is used
	ParsingArguments AdjustPaForCollecting(const ParsingArguments& pa)
	{
		auto invokerPa = pa;
		if (!pa.taContext && pa.parentDeclType)
		{
			auto& di = pa.parentDeclType->GetDeclInstant();
			invokerPa.parentDeclType = di.parentDeclType;
			invokerPa.taContext = di.taContext.Obj();
		}
		return invokerPa;
	}

	void ResolveFunctionParameters(
		const ParsingArguments& invokerPa,				// context
		TypeTsysList& parameterAssignment,				// store function argument to offered argument map, nullptr indicates the default value is applied
		const TemplateArgumentContext& knownArguments,	// all assigned template arguments
		const SortedList<Symbol*>& argumentSymbols,		// symbols of all template arguments
		FunctionType* functionType,						// argument information
		Array<ExprTsysItem>& argumentTypes,				// (index of unpacked)		offered argument (unpacked)
		SortedList<vint>& boundedAnys					// (value of unpacked)		for each offered argument that is any_t, and it means unknown variadic arguments, instead of an unknown type
	)
	{
		auto adjustedPa = AdjustPaForCollecting(invokerPa);

		vint functionParameterCount = functionType->parameters.Count();
		vint passedParameterCount = functionParameterCount + (functionType->ellipsis ? 1 : 0);
		Array<vint> knownPackSizes(passedParameterCount);
		for (vint i = 0; i < knownPackSizes.Count(); i++)
		{
			knownPackSizes[i] = -1;
		}

		// calculate all variadic function arguments that with known pack size
		for (vint i = 0; i < functionType->parameters.Count(); i++)
		{
			auto& parameter = functionType->parameters[i];
			if (parameter.isVariadic)
			{
				SortedList<Type*> involvedTypes;
				SortedList<Expr*> involvedExprs;
				CollectFreeTypes(adjustedPa, true, parameter.item->type, nullptr, false, argumentSymbols, involvedTypes, involvedExprs);
				knownPackSizes[i] = CalculateParameterPackSize(adjustedPa, knownArguments, involvedTypes, involvedExprs);
			}
			else
			{
				knownPackSizes[i] = 1;
			}
		}

		// calculate how to assign offered arguments to function arguments
		// gpaMappings will contains decisions for every template arguments
		GpaList gpaMappings;
		CalculateGpa(gpaMappings, argumentTypes.Count(), boundedAnys, 0, false, passedParameterCount, knownPackSizes,
			[functionType](vint index) { return index == functionType->parameters.Count() ? functionType->ellipsis : functionType->parameters[index].isVariadic; },
			[functionType](vint index) { return index == functionType->parameters.Count() ? false : (bool)functionType->parameters[index].item->initializer; }
		);
		AssignParameterAssignment(adjustedPa, functionParameterCount, parameterAssignment, gpaMappings, argumentTypes);
	}

	void ResolveGenericTypeParameters(
		const ParsingArguments& invokerPa,				// context
		TypeTsysList& parameterAssignment,				// store function argument to offered argument map, nullptr indicates the default value is applied
		const TemplateArgumentContext& knownArguments,	// all assigned template arguments
		const SortedList<Symbol*>& argumentSymbols,		// symbols of all template arguments
		GenericType* genericType,						// argument information
		Array<ExprTsysItem>& argumentTypes,				// (index of unpacked)		offered argument (unpacked)
		SortedList<vint>& boundedAnys					// (value of unpacked)		for each offered argument that is any_t, and it means unknown variadic arguments, instead of an unknown type
	)
	{
		auto adjustedPa = AdjustPaForCollecting(invokerPa);

		vint genericParameterCount = genericType->arguments.Count();
		vint passedParameterCount = genericParameterCount + 1;
		Array<vint> knownPackSizes(passedParameterCount);
		for (vint i = 0; i < knownPackSizes.Count(); i++)
		{
			knownPackSizes[i] = -1;
		}

		// calculate all variadic function arguments that with known pack size
		for (vint i = 0; i < genericType->arguments.Count(); i++)
		{
			auto& argument = genericType->arguments[i];
			if (argument.isVariadic)
			{
				SortedList<Type*> involvedTypes;
				SortedList<Expr*> involvedExprs;
				CollectFreeTypes(adjustedPa, true, argument.item.type, argument.item.expr, false, argumentSymbols, involvedTypes, involvedExprs);
				knownPackSizes[i] = CalculateParameterPackSize(adjustedPa, knownArguments, involvedTypes, involvedExprs);
			}
			else
			{
				knownPackSizes[i] = 1;
			}
		}

		// calculate how to assign offered arguments to generic type arguments
		// gpaMappings will contains decisions for every template arguments
		// set allowPartialApply to true because, arguments of genericType could be incomplete, but argumentTypes are always complete because it comes from a ITsys*
		GpaList gpaMappings;
		CalculateGpa(gpaMappings, argumentTypes.Count(), boundedAnys, 0, true, passedParameterCount, knownPackSizes,
			[genericType](vint index) { return index == genericType->arguments.Count() ? true : genericType->arguments[index].isVariadic; },
			[](vint index) { return false; }
		);
		AssignParameterAssignment(adjustedPa, genericParameterCount, parameterAssignment, gpaMappings, argumentTypes);
	}
}