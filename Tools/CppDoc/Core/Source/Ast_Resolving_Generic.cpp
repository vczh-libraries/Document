#include "Ast_Resolving.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	CreateGenericFunctionHeader: Calculate enough information to create a generic function type
	***********************************************************************/

	void CreateGenericFunctionHeader(Ptr<TemplateSpec> spec, TypeTsysList& params, TsysGenericFunction& genericFunction)
	{
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

		if (genericFunction->GetParamCount() < argumentTypes.Count())
		{
			return false;
		}
		else if (genericFunction->GetParamCount() > argumentTypes.Count())
		{
			if (spec)
			{
				for (vint i = argumentTypes.Count(); i < spec->arguments.Count(); i++)
				{
					auto& argument = spec->arguments[i];
					switch (argument.argumentType)
					{
					case CppTemplateArgumentType::Value:
						if (!argument.expr) return false;
						break;
					default:
						if (!argument.type) return false;
					}
				}
			}
			else
			{
				return false;
			}
		}

		for (vint i = 0; i < genericFunction->GetParamCount(); i++)
		{
			auto pattern = genericFunction->GetParam(i);
			bool useDefault = argumentTypes.Count() <= i;
			if (!useDefault)
			{
				if ((pattern == nullptr) ^ (argumentTypes[i] == nullptr))
				{
					return false;
				}
			}

			if (pattern != nullptr)
			{
				if (useDefault)
				{
					TypeTsysList argTypes;
					TypeToTsys(pa.WithContext(genericSymbol), spec->arguments[i].type, argTypes, newGaContext);

					for (vint j = 0; j < argTypes.Count(); j++)
					{
						newGaContext->arguments.Add(pattern, argTypes[j]);
					}
				}
				else
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
		}
		return true;
	}
}