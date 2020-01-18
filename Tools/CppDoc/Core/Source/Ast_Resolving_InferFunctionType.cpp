#include "Ast_Resolving.h"

namespace symbol_type_resolving
{

	/***********************************************************************
	InferFunctionType: Perform type inferencing for template function using both offered template and function arguments
	***********************************************************************/

	void InferTemplateArgumentsForFunctionType(const ParsingArguments& pa, ExprTsysItem functionItem, Ptr<FunctionType> functionType, Array<ExprTsysItem>& argTypes, SortedList<vint>& boundedAnys, TemplateArgumentContext& taContext)
	{
		// don't care about arguments for ellipsis
		for (vint i = 0; i < functionType->parameters.Count(); i++)
		{
			// see if any variadic value arguments can be determined
			// variadic value argument only care about the number of values
			auto parameter = functionType->parameters[i];
			if (parameter.isVariadic)
			{
				// TODO: not implemented
				throw NotConvertableException();
			}
			else
			{
			}
		}
		throw NotConvertableException();
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
						auto gfi = functionItem.tsys->GetGenericFunction();
						if (gfi.filledArguments != 0) throw NotConvertableException();

						List<ITsys*> parameterAssignment;
						// cannot pass Ptr<FunctionType> to this function since the last filled argument could be variadic
						// known variadic function argument should be treated as separated arguments
						// ParsingArguments need to be adjusted so that we can evaluate each parameter type
						ResolveFunctionParameters(pa, parameterAssignment, functionType, argTypes, boundedAnys);

						auto inferPa = pa.AdjustForDecl(gfi.declSymbol, gfi.parentDeclType, false);

						TemplateArgumentContext taContext;
						for (vint i = 0; i < gfi.spec->arguments.Count(); i++)
						{
							auto argument = gfi.spec->arguments[i];
							auto pattern = GetTemplateArgumentKey(argument, pa.tsys.Obj());
							if (i < gfi.filledArguments)
							{
								taContext.arguments.Add(pattern, functionItem.tsys->GetParam(i));
							}
							else if (argument.argumentType == CppTemplateArgumentType::Value)
							{
								if (argument.ellipsis)
								{
									// TODO: not implemented
									throw NotConvertableException();
								}
								else
								{
									taContext.arguments.Add(pattern, nullptr);
								}
							}
						}

						InferTemplateArgumentsForFunctionType(inferPa, functionItem, functionType, argTypes, boundedAnys, taContext);
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
							throw NotConvertableException();
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