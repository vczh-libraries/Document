#include "Ast_EvaluateSymbol_Shared.h"
#include "Ast_Type.h"
#include "Parser.h"

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
		switch (ev.progress)
		{
		case symbol_component::EvaluationProgress::Evaluated:
			// the return statement has recursively called this function with the same template arguments
			// nothing need to be done
			return;
		case symbol_component::EvaluationProgress::NotEvaluated:
		case symbol_component::EvaluationProgress::RecursiveFound:
			// this function can only be called indirectly in EvaluateFuncSymbol, so NotEvaluated is not possible
			// RecursiveFound will not call EvaluateStat, so this is not possible either
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
		// template function recursion could cause this function with the same template arguments to be evaluated again
		auto eval = ProcessArguments(invokerPa, funcDecl, funcDecl->templateSpec, parentDeclType, argumentsToApply, true);
		if (eval)
		{
			// Since types of full specialized functions should be exactly the same with the primary
			// so nothing is needed for type inferencing
			if (eval.ev.progress == symbol_component::EvaluationProgress::RecursiveFound)
			{
				// recursive call is found, the return type is any_t
				TypeTsysList processedReturnTypes;
				processedReturnTypes.Add(eval.declPa.tsys->Any());

				TypeToTsysAndReplaceFunctionReturnType(
					invokerPa,
					funcDecl->type,
					processedReturnTypes,
					eval.evaluatedTypes,
					IsMemberFunction(funcDecl)
				);

				return FinishEvaluatingPotentialGenericSymbol(eval.declPa, funcDecl, funcDecl->templateSpec, argumentsToApply);
			}
			else if (funcDecl->needResolveTypeFromStatement)
			{
				if (auto rootFuncDecl = dynamic_cast<FunctionDeclaration*>(funcDecl))
				{
					EnsureFunctionBodyParsed(rootFuncDecl);
					EvaluateStat(eval.declPa, rootFuncDecl->statement, true, argumentsToApply);

					// EvaluateStat could change ev.progress when recursion happens
					if (eval.evaluatedTypes.Count() == 0 && eval.ev.progress == symbol_component::EvaluationProgress::Evaluating)
					{
						// no return statement is found, the return type is void
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
						// FinishEvaluatingPotentialGenericSymbol has been called in SetFuncTypeByReturnStat
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