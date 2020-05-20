#include "EvaluateSymbol_Shared.h"

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
		auto eval = ProcessArguments(invokerPa, usingDecl, usingDecl->templateSpec, parentDeclType, argumentsToApply, true);
		if (eval)
		{
			if (eval.ev.progress == symbol_component::EvaluationProgress::RecursiveFound)
			{
				// recursive call is found, the return type is any_t
				eval.evaluatedTypes.Add(eval.declPa.tsys->Any());
			}
			else
			{
				TypeTsysList processedTypes;
				TypeToTsysNoVta(eval.declPa, usingDecl->type, processedTypes);

				// TypeToTsysNoVta could have filled eval.evaluatedTypes when recursion happens
				eval.evaluatedTypes.Clear();
				CopyFrom(eval.evaluatedTypes, processedTypes);
			}
			return FinishEvaluatingPotentialGenericSymbol(eval.declPa, usingDecl, usingDecl->templateSpec, argumentsToApply);
		}
		else
		{
			return eval.evaluatedTypes;
		}
	}
}