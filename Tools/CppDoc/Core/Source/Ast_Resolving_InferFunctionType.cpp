#include "Ast_Resolving.h"

namespace symbol_type_resolving
{

	/***********************************************************************
	InferFunctionType: Perform type inferencing for template function using both offered template and function arguments
	***********************************************************************/

	Nullable<ExprTsysItem> InferFunctionType(const ParsingArguments& pa, ExprTsysItem funcType, Array<ExprTsysItem>& argTypes, SortedList<vint>& boundedAnys)
	{
		switch (funcType.tsys->GetType())
		{
		case TsysType::Function:
			return funcType;
		case TsysType::LRef:
		case TsysType::RRef:
		case TsysType::CV:
		case TsysType::Ptr:
			return InferFunctionType(pa, { funcType,funcType.tsys->GetElement() }, argTypes, boundedAnys);
		case TsysType::GenericFunction:
			if (auto symbol = funcType.symbol)
			{
				if (auto decl = symbol->GetAnyForwardDecl<ForwardFunctionDeclaration>())
				{
					if (auto type = GetTypeWithoutMemberAndCC(decl->type).Cast<FunctionType>())
					{
						auto gfi = funcType.tsys->GetGenericFunction();
						if (gfi.filledArguments != 0) throw NotConvertableException();

						List<ITsys*> parameterAssignment;
						// cannot pass Ptr<FunctionType> to this function since the last filled argument could be variadic
						// known variadic function argument should be treated as separated arguments
						// ParsingArguments need to be adjusted so that we can evaluate each parameter type
						ResolveFunctionParameters(pa, parameterAssignment, type, argTypes, boundedAnys);
						throw NotConvertableException();
					}
				}
			}
		default:
			if (funcType.tsys->IsUnknownType())
			{
				return funcType;
			}
			return {};
		}
	}
}