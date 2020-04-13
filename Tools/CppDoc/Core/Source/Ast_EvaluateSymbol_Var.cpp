#include "Ast_EvaluateSymbol.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	EvaluateVarSymbol: Evaluate the declared type for a variable
	***********************************************************************/

	TypeTsysList& EvaluateVarSymbol(
		const ParsingArguments& invokerPa,
		ForwardVariableDeclaration* varDecl,
		ITsys* parentDeclType
	)
	{
		auto eval = ProcessArguments(invokerPa, varDecl, nullptr, parentDeclType, nullptr);
		if(eval)
		{
			if (varDecl->needResolveTypeFromInitializer)
			{
				if (auto rootVarDecl = dynamic_cast<VariableDeclaration*>(varDecl))
				{
					if (!rootVarDecl->initializer)
					{
						throw TypeCheckerException();
					}

					ExprTsysList types;
					ExprToTsysNoVta(eval.declPa, rootVarDecl->initializer->arguments[0].item, types);

					for (vint k = 0; k < types.Count(); k++)
					{
						auto type = ResolvePendingType(eval.declPa, rootVarDecl->type, types[k]);
						if (!eval.evaluatedTypes.Contains(type))
						{
							eval.evaluatedTypes.Add(type);
						}
					}
				}
				else
				{
					throw TypeCheckerException();
				}
			}
			else
			{
				TypeToTsysNoVta(eval.declPa, varDecl->type, eval.evaluatedTypes);
			}

			eval.ev.progress = symbol_component::EvaluationProgress::Evaluated;
			return eval.evaluatedTypes;
		}
		else
		{
			return eval.evaluatedTypes;
		}
	}
}