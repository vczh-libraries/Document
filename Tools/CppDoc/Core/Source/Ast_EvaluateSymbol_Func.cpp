#include "Ast_EvaluateSymbol.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	EvaluateFuncSymbol: Evaluate the declared type for a function
	***********************************************************************/

	bool IsMemberFunction(ForwardFunctionDeclaration* funcDecl)
	{
		auto symbol = funcDecl->symbol;
		if (IsStaticSymbol<ForwardFunctionDeclaration>(symbol))
		{
			return false;
		}

		if (auto parent = symbol->GetParentScope())
		{
			if (parent->GetImplDecl_NFb<ClassDeclaration>())
			{
				return true;
			}
		}

		return false;
	}

	void SetFuncTypeByReturnStat(
		const ParsingArguments& pa,
		FunctionDeclaration* funcDecl,
		TypeTsysList& returnTypes,
		TemplateArgumentContext* argumentsToApply
	)
	{
		auto symbol = funcDecl->symbol;
		auto& ev = GetCorrectEvaluation(pa, funcDecl, funcDecl->templateSpec, argumentsToApply);
		if (ev.progress != symbol_component::EvaluationProgress::Evaluating)
		{
			throw TypeCheckerException();
		}

		auto newPa = pa.WithScope(symbol);
		auto& evaluatedTypes = ev.Get();

		TypeTsysList processedReturnTypes;
		{
			auto funcType = GetTypeWithoutMemberAndCC(funcDecl->type).Cast<FunctionType>();
			auto pendingType = funcType->decoratorReturnType ? funcType->decoratorReturnType : funcType->returnType;
			for (vint i = 0; i < returnTypes.Count(); i++)
			{
				auto tsys = ResolvePendingType(newPa, pendingType, { nullptr,ExprTsysType::PRValue,returnTypes[i] });
				if (!processedReturnTypes.Contains(tsys))
				{
					processedReturnTypes.Add(tsys);
				}
			}
		}
		TypeToTsysAndReplaceFunctionReturnType(
			newPa,
			funcDecl->type,
			processedReturnTypes,
			evaluatedTypes,
			IsMemberFunction(funcDecl)
		);

		FinishEvaluatingPotentialGenericSymbol(pa, funcDecl, funcDecl->templateSpec, argumentsToApply);
	}

	TypeTsysList& EvaluateFuncSymbol(
		const ParsingArguments& invokerPa,
		ForwardFunctionDeclaration* funcDecl,
		ITsys* parentDeclType,
		TemplateArgumentContext* argumentsToApply
	)
	{
		auto eval = ProcessArguments(invokerPa, funcDecl, funcDecl->templateSpec, parentDeclType, argumentsToApply);
		if (eval)
		{
			// Since types of full specialized functions should be exactly the same with the primary
			// so nothing is needed for type inferencing
			if (funcDecl->needResolveTypeFromStatement)
			{
				if (auto rootFuncDecl = dynamic_cast<FunctionDeclaration*>(funcDecl))
				{
					EnsureFunctionBodyParsed(rootFuncDecl);
					EvaluateStat(eval.declPa, rootFuncDecl->statement, true, argumentsToApply);

					if (eval.evaluatedTypes.Count() == 0)
					{
						TypeTsysList processedReturnTypes;
						processedReturnTypes.Add(eval.declPa.tsys->Void());

						TypeToTsysAndReplaceFunctionReturnType(
							invokerPa,
							funcDecl->type,
							processedReturnTypes,
							eval.evaluatedTypes,
							IsMemberFunction(funcDecl)
						);

						return FinishEvaluatingPotentialGenericSymbol(eval.declPa, funcDecl, funcDecl->templateSpec, argumentsToApply);
					}
					else
					{
						return eval.evaluatedTypes;
					}
				}
				else
				{
					throw TypeCheckerException();
				}
			}
			else
			{
				TypeToTsysNoVta(eval.declPa, funcDecl->type, eval.evaluatedTypes, TypeToTsysConfig::MemberOf(IsMemberFunction(funcDecl)));
				return FinishEvaluatingPotentialGenericSymbol(eval.declPa, funcDecl, funcDecl->templateSpec, argumentsToApply);
			}
		}
		else
		{
			return eval.evaluatedTypes;
		}
	}
}