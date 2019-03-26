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
}