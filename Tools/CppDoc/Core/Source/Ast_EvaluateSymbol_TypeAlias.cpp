#include "Ast_EvaluateSymbol_Shared.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	EvaluateTypeAliasSymbol: Evaluate the declared type for an alias
	***********************************************************************/

	TypeTsysList& EvaluateTypeAliasSymbol(
		const ParsingArguments& invokerPa,
		TypeAliasDeclaration* usingDecl,
		ITsys* parentDeclType,
		TemplateArgumentContext* argumentsToApply
	)
	{
		auto eval = ProcessArguments(invokerPa, usingDecl, usingDecl->templateSpec, parentDeclType, argumentsToApply);
		if (eval)
		{
			TypeToTsysNoVta(eval.declPa, usingDecl->type, eval.evaluatedTypes);
			return FinishEvaluatingPotentialGenericSymbol(eval.declPa, usingDecl, usingDecl->templateSpec, argumentsToApply);
		}
		else
		{
			return eval.evaluatedTypes;
		}
	}
}