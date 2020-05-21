#include "Ast_Resolving.h"
#include "EvaluateSymbol.h"

/***********************************************************************
ReplaceGenericArgsInClass
***********************************************************************/

ITsys* ReplaceGenericArgsInClass(const ParsingArguments& pa, ITsys* decoratedClassType)
{
	return decoratedClassType->ReplaceGenericArgs(pa);
}

/***********************************************************************
ReplaceGenericArg
***********************************************************************/

ITsys* ReplaceGenericArg(const ParsingArguments& pa, ITsys* genericArgType)
{
	ITsys* result = nullptr;
	if (pa.TryGetReplacedGenericArg(genericArgType, result))
	{
		return result;
	}
	else
	{
		return genericArgType;
	}
}