#include "Ast_Resolving.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	CreateGenericFunctionHeader: Calculate enough information to create a generic function type
	***********************************************************************/

	void CreateGenericFunctionHeader(Ptr<TemplateSpec> spec, TypeTsysList& params, TsysGenericFunction& genericFunction)
	{
		genericFunction.variadicArgumentIndex = -1;
		for (vint i = 0; i < spec->arguments.Count(); i++)
		{
			auto argument = spec->arguments[i];
			if (argument.argumentType == CppTemplateArgumentType::Value)
			{
				params.Add(nullptr);
			}
			else
			{
				genericFunction.arguments.Add(argument.argumentSymbol);
				params.Add(argument.argumentSymbol->evaluation.Get()[0]);
			}

			if (argument.ellipsis)
			{
				if (genericFunction.variadicArgumentIndex == -1)
				{
					genericFunction.variadicArgumentIndex = i;
				}
				else
				{
					throw NotConvertableException();
				}
			}
		}

		if (genericFunction.variadicArgumentIndex != -1)
		{
			for (vint i = 0; i < spec->arguments.Count(); i++)
			{
				auto argument = spec->arguments[i];
				switch (argument.argumentType)
				{
				case CppTemplateArgumentType::HighLevelType:
				case CppTemplateArgumentType::Type:
					if (argument.type) throw NotConvertableException();
					break;
				case CppTemplateArgumentType::Value:
					if (argument.expr) throw NotConvertableException();
					break;
				}
			}
		}
	}

	/***********************************************************************
	ResolveGenericArguments: Calculate types from generic arguments
	***********************************************************************/

	void ResolveGenericArguments(const ParsingArguments& pa, List<GenericArgument>& arguments, Array<Ptr<TypeTsysList>>& argumentTypes, GenericArgContext* gaContext)
	{
		argumentTypes.Resize(arguments.Count());
		for (vint i = 0; i < arguments.Count(); i++)
		{
			auto argument = arguments[i];
			if (argument.type)
			{
				argumentTypes[i] = MakePtr<TypeTsysList>();
				TypeToTsys(pa, argument.type, *argumentTypes[i].Obj(), gaContext);
			}
		}
	}

	/***********************************************************************
	ResolveGenericParameters: Calculate generic parameter types by matching arguments to patterens
	***********************************************************************/

	void EnsureGenericParameterAndArgumentMatched(ITsys* parameter, ITsys* argument)
	{
		if (parameter->GetParamCount() != argument->GetParamCount())
		{
			throw NotConvertableException();
		}
		for (vint i = 0; i < parameter->GetParamCount(); i++)
		{
			auto nestedParameter = parameter->GetParam(i);
			auto nestedArgument = parameter->GetParam(i);
			if ((nestedParameter == nullptr) ^ (nestedArgument == nullptr))
			{
				throw NotConvertableException();
			}

			if (nestedParameter)
			{
				switch (nestedParameter->GetType())
				{
				case TsysType::GenericArg:
					if (nestedArgument->GetType() == TsysType::GenericFunction)
					{
						throw NotConvertableException();
					}
					break;
				case TsysType::GenericFunction:
					if (nestedArgument->GetType() != TsysType::GenericFunction)
					{
						throw NotConvertableException();
					}
					EnsureGenericParameterAndArgumentMatched(nestedParameter, nestedArgument);
					break;
				default:
					// there is not specialization for high-level template argument
					throw NotConvertableException();
				}
			}
		}
	}

	bool ResolveGenericParameters(const ParsingArguments& pa, ITsys* genericFunction, Array<Ptr<TypeTsysList>>& argumentTypes, GenericArgContext* newGaContext)
	{
		if (genericFunction->GetType() != TsysType::GenericFunction)
		{
			return false;
		}

		Ptr<TemplateSpec> spec;
		auto genericSymbol = genericFunction->GetGenericFunction().declSymbol;
		if (genericSymbol)
		{
			if (auto usingDecl = genericSymbol->GetAnyForwardDecl<UsingDeclaration>())
			{
				spec = usingDecl->templateSpec;
			}
		}

		vint minCount = -1;
		vint maxCount = -1;
		vint variadicArgumentIndex = genericFunction->GetGenericFunction().variadicArgumentIndex;
		if (variadicArgumentIndex == -1)
		{
			vint defaultCount = 0;
			if (spec)
			{
				for (vint i = spec->arguments.Count() - 1; i >= 0; i--)
				{
					auto argument = spec->arguments[i];
					switch (argument.argumentType)
					{
					case CppTemplateArgumentType::HighLevelType:
					case CppTemplateArgumentType::Type:
						if (!argument.type) goto STOP_COUNTING_DEFAULT;
						defaultCount++;
						break;
					case CppTemplateArgumentType::Value:
						if (!argument.expr) goto STOP_COUNTING_DEFAULT;
						defaultCount++;
						break;
					}
				}
			}
		STOP_COUNTING_DEFAULT:
			maxCount = genericFunction->GetParamCount();
			minCount = maxCount - defaultCount;
		}
		else
		{
			// default values and variadic template arguments are not allowed to mix together
			minCount = genericFunction->GetParamCount() - 1;
		}

		if (argumentTypes.Count() < minCount || (maxCount != -1 && argumentTypes.Count() > maxCount))
		{
			return false;
		}

		// -1, -1: default value
		// X, X: map to one argument
		// X, X-1: map to no arguments (variadic)
		// X, Y: map to multiple arguments (variadic)
		List<Tuple<vint, vint>> parameterToArgumentMappings;
		if (variadicArgumentIndex == -1)
		{
			for (vint i = 0; i < genericFunction->GetParamCount(); i++)
			{
				if (i < argumentTypes.Count())
				{
					parameterToArgumentMappings.Add({ i,i });
				}
				else
				{
					parameterToArgumentMappings.Add({ -1,-1 });
				}
			}
		}
		else
		{
			vint delta = argumentTypes.Count() - genericFunction->GetParamCount();
			for (vint i = 0; i < variadicArgumentIndex; i++)
			{
				parameterToArgumentMappings.Add({ i,i });
			}
			parameterToArgumentMappings.Add({ variadicArgumentIndex,variadicArgumentIndex + delta });
			for (vint i = variadicArgumentIndex + 1; i < genericFunction->GetParamCount(); i++)
			{
				parameterToArgumentMappings.Add({ i + delta,i + delta });
			}
		}

		for (vint i = 0; i < genericFunction->GetParamCount(); i++)
		{
			auto mappings = parameterToArgumentMappings[i];
			auto pattern = genericFunction->GetParam(i);
			if (mappings.f0 == -1)
			{
				if (pattern != nullptr)
				{
					TypeTsysList argTypes;
					TypeToTsys(pa.WithContext(genericSymbol), spec->arguments[i].type, argTypes, newGaContext);

					for (vint j = 0; j < argTypes.Count(); j++)
					{
						newGaContext->arguments.Add(pattern, argTypes[j]);
					}
				}
			}
			else if (mappings.f0 == mappings.f1)
			{
				if (pattern != nullptr)
				{
					auto& argTypes = *argumentTypes[i].Obj();
					switch (pattern->GetType())
					{
					case TsysType::GenericArg:
						for (vint j = 0; j < argTypes.Count(); j++)
						{
							auto tsys = argTypes[j];
							if (tsys->GetType() == TsysType::GenericFunction)
							{
								throw NotConvertableException();
							}
						}
						break;
					case TsysType::GenericFunction:
						for (vint j = 0; j < argTypes.Count(); j++)
						{
							auto tsys = argTypes[j];
							if (tsys->GetType() != TsysType::GenericFunction)
							{
								throw NotConvertableException();
							}
							EnsureGenericParameterAndArgumentMatched(pattern, tsys);
						}
						break;
					default:
						// until class specialization begins to develop, this should always not happen
						throw NotConvertableException();
					}

					for (vint j = 0; j < argTypes.Count(); j++)
					{
						newGaContext->arguments.Add(pattern, argTypes[j]);
					}
				}
			}
			else if (mappings.f0 == mappings.f1 + 1)
			{
				if (pattern != nullptr)
				{
					Array<ExprTsysItem> items;
					auto init = pa.tsys->InitOf(items);
					newGaContext->arguments.Add(pattern, init);
				}
			}
			else
			{
				if (pattern != nullptr)
				{
					// TODO: convert multiple types to {types...}
					throw NotConvertableException();
				}
			}
		}
		return true;
	}
}