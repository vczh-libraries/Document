#include "Ast_EvaluateSymbol_Shared.h"
#include "Ast_Resolving_IFT.h"

using namespace infer_function_type;

namespace symbol_type_resolving
{
	/***********************************************************************
	EvaluateValueAliasSymbol: Evaluate the declared value for an alias
	***********************************************************************/

	void EvaluateValueAliasSymbolInternal(
		const ParsingArguments& declPa,
		TypeTsysList& result,
		ValueAliasDeclaration* usingDecl
	)
	{
		if (usingDecl->needResolveTypeFromInitializer)
		{
			ExprTsysList tsys;
			ExprToTsysNoVta(declPa, usingDecl->expr, tsys);
			for (vint i = 0; i < tsys.Count(); i++)
			{
				auto entityType = tsys[i].tsys;
				if (entityType->GetType() == TsysType::Zero)
				{
					entityType = declPa.tsys->Int();
				}
				if (!result.Contains(entityType))
				{
					result.Add(entityType);
				}
			}
		}
		else
		{
			TypeToTsysNoVta(declPa, usingDecl->type, result);
		}
	}

	TypeTsysList& EvaluateValueAliasSymbol(
		const ParsingArguments& invokerPa,
		ValueAliasDeclaration* usingDecl,
		ITsys* parentDeclType,
		TemplateArgumentContext* argumentsToApply
	)
	{
		auto eval = ProcessArguments(invokerPa, usingDecl, usingDecl->templateSpec, parentDeclType, argumentsToApply);
		if (eval)
		{
			if (!eval.symbol->IsPSPrimary_NF())
			{
				EvaluateValueAliasSymbolInternal(eval.declPa, eval.evaluatedTypes, usingDecl);
			}
			else if (argumentsToApply)
			{
				auto psPa = eval.declPa;
				psPa.taContext = psPa.taContext->parent;
				Dictionary<Symbol*, Ptr<TemplateArgumentContext>> psResult;
				InferPartialSpecializationPrimary<ValueAliasDeclaration>(psPa, psResult, eval.symbol, psPa.parentDeclType, argumentsToApply);

				if (psResult.Count() == 0 || IsValuableTaContextWithMatchedPSChildren(argumentsToApply))
				{
					EvaluateValueAliasSymbolInternal(eval.declPa, eval.evaluatedTypes, usingDecl);
				}

				for (vint i = 0; i < psResult.Count(); i++)
				{
					auto declSymbol = psResult.Keys()[i];
					auto decl = declSymbol->GetAnyForwardDecl<ValueAliasDeclaration>();
					auto taContext = psResult.Values()[i];

					auto declPa = psPa;
					taContext->parent = declPa.taContext;
					declPa.taContext = taContext.Obj();
					EvaluateValueAliasSymbolInternal(declPa, eval.evaluatedTypes, decl.Obj());
				}
			}
			else
			{
				TypeTsysList result;
				EvaluateValueAliasSymbolInternal(eval.declPa, result, usingDecl);
				eval.evaluatedTypes.Add(invokerPa.tsys->Any());
			}

			for (vint i = 0; i < eval.evaluatedTypes.Count(); i++)
			{
				eval.evaluatedTypes[i] = eval.evaluatedTypes[i]->CVOf({ true,false })->LRefOf();
			}

			return FinishEvaluatingPotentialGenericSymbol(eval.declPa, usingDecl, usingDecl->templateSpec, argumentsToApply);
		}
		else
		{
			return eval.evaluatedTypes;
		}
	}
}