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
	return genericArgType->ReplaceGenericArgs(pa);
}