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
	ResolveGenericParameters: Calculate generic parameter types by matching arguments to patterens
	***********************************************************************/

	void EnsureGenericNormalParameterAndArgumentMatched(ITsys* parameter, ITsys* argument);
	void EnsureGenericFunctionParameterAndArgumentMatched(ITsys* parameter, ITsys* argument);

	void EnsureGenericNormalParameterAndArgumentMatched(ITsys* parameter, ITsys* argument)
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
			auto nestedArgument = parameter->GetParam(i);
			if ((nestedParameter == nullptr) ^ (nestedArgument == nullptr))
			{
				throw NotConvertableException();
			}

			if (nestedParameter)
			{
				EnsureGenericNormalParameterAndArgumentMatched(nestedParameter, nestedArgument);
			}
		}
	}

	bool ResolveGenericParameters(const ParsingArguments& pa, ITsys* genericFunction, Array<ExprTsysItem>& argumentTypes, Array<bool>& isTypes, vint offset, GenericArgContext* newGaContext)
	{
		if (genericFunction->GetType() != TsysType::GenericFunction)
		{
			return false;
		}

		Ptr<TemplateSpec> spec;
		auto genericSymbol = genericFunction->GetGenericFunction().declSymbol;
		if (genericSymbol)
		{
			if (auto typeAliasDecl = genericSymbol->GetAnyForwardDecl<TypeAliasDeclaration>())
			{
				spec = typeAliasDecl->templateSpec;
			}
			else if (auto valueAliasDecl = genericSymbol->GetAnyForwardDecl<ValueAliasDeclaration>())
			{
				spec = valueAliasDecl->templateSpec;
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

		vint inputArgumentCount = argumentTypes.Count() - offset;
		if (inputArgumentCount < minCount || (maxCount != -1 && inputArgumentCount > maxCount))
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
				if (i < inputArgumentCount)
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
			vint delta = inputArgumentCount - genericFunction->GetParamCount();
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
				// -1, -1: default value
				if (pattern != nullptr)
				{
					if (spec->arguments[i].argumentType == CppTemplateArgumentType::Value)
					{
						throw NotConvertableException();
					}
					TypeTsysList argTypes;
					TypeToTsysNoVta(pa.WithContext(genericSymbol), spec->arguments[i].type, argTypes, newGaContext);

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
				}
			}
			else if (mappings.f0 == mappings.f1 && (variadicArgumentIndex == -1 || variadicArgumentIndex != mappings.f0))
			{
				// X, X: map to one argument
				if ((pattern != nullptr) != isTypes[i + offset])
				{
					throw NotConvertableException();
				}

				if (pattern != nullptr)
				{
					auto item = argumentTypes[i + offset];
					EnsureGenericNormalParameterAndArgumentMatched(pattern, item.tsys);
					newGaContext->arguments.Add(pattern, item.tsys);
				}
			}
			else if (mappings.f0 == mappings.f1 + 1)
			{
				// X, X-1: map to no arguments (variadic)
				if (pattern != nullptr)
				{
					Array<ExprTsysItem> items;
					auto init = pa.tsys->InitOf(items);
					newGaContext->arguments.Add(pattern, init);
				}
			}
			else
			{
				// X, Y: map to multiple arguments (variadic)
				for (vint j = mappings.f0; j <= mappings.f1; j++)
				{
					if ((pattern != nullptr) != isTypes[j + offset])
					{
						throw NotConvertableException();
					}
				}

				if (pattern != nullptr)
				{
					Array<ExprTsysItem> items(mappings.f1 - mappings.f0 + 1);

					for (vint j = mappings.f0; j <= mappings.f1; j++)
					{
						auto item = argumentTypes[j + offset];
						EnsureGenericNormalParameterAndArgumentMatched(pattern, item.tsys);
						items[j - mappings.f0] = item;
					}

					auto init = pa.tsys->InitOf(items);
					newGaContext->arguments.Add(pattern, init);
				}
			}
		}
		return true;
	}
}