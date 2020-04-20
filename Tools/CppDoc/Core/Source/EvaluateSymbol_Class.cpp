#include "EvaluateSymbol_Shared.h"
#include "Symbol_TemplateSpec.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	EvaluateForwardClassSymbol: Evaluate the declared type for a class
	***********************************************************************/

	TypeTsysList& EvaluateForwardClassSymbol(
		const ParsingArguments& invokerPa,
		ForwardClassDeclaration* classDecl,
		ITsys* parentDeclType,
		TemplateArgumentContext* argumentsToApply
	)
	{
		auto eval = ProcessArguments(invokerPa, classDecl, classDecl->templateSpec, parentDeclType, argumentsToApply);
		if (eval)
		{
			for (vint i = 0; i < eval.evaluatedTypes.Count(); i++)
			{
				auto tsys = eval.evaluatedTypes[i];
				if (tsys->GetType() == TsysType::GenericFunction)
				{
					auto expect = GetTemplateArgumentKey(classDecl->templateSpec->arguments[0], eval.declPa.tsys.Obj());
					auto actual = tsys->GetParam(0);
					if (expect != actual)
					{
						eval.notEvaluated = true;
						eval.evaluatedTypes.Clear();
						eval.ev.progress = symbol_component::EvaluationProgress::Evaluating;
					}
				}
			}
		}

		if (eval)
		{
			if (classDecl->templateSpec)
			{
				Array<ITsys*> params(classDecl->templateSpec->arguments.Count());
				for (vint i = 0; i < classDecl->templateSpec->arguments.Count(); i++)
				{
					params[i] = GetTemplateArgumentKey(classDecl->templateSpec->arguments[i], eval.declPa.tsys.Obj());
				}
				auto diTsys = eval.declPa.tsys->DeclInstantOf(eval.symbol, &params, eval.declPa.parentDeclType);
				eval.evaluatedTypes.Add(diTsys);
			}
			else
			{
				if (eval.declPa.parentDeclType)
				{
					eval.evaluatedTypes.Add(eval.declPa.tsys->DeclInstantOf(eval.symbol, nullptr, eval.declPa.parentDeclType));
				}
				else
				{
					eval.evaluatedTypes.Add(eval.declPa.tsys->DeclOf(eval.symbol));
				}
			}

			if (argumentsToApply)
			{
				eval.evaluatedTypes[0] = eval.evaluatedTypes[0]->ReplaceGenericArgs(eval.declPa);
			}
			return FinishEvaluatingPotentialGenericSymbol(eval.declPa, classDecl, classDecl->templateSpec, argumentsToApply);
		}
		else
		{
			return eval.evaluatedTypes;
		}
	}

	/***********************************************************************
	EvaluateClassSymbol: Evaluate the declared type and base types for a class
	***********************************************************************/

	symbol_component::Evaluation& EvaluateClassSymbol(
		const ParsingArguments& invokerPa,
		ClassDeclaration* classDecl,
		ITsys* parentDeclType,
		TemplateArgumentContext* argumentsToApply
	)
	{
		EvaluateForwardClassSymbol(invokerPa, classDecl, parentDeclType, argumentsToApply);
		auto eval = ProcessArguments(invokerPa, classDecl, classDecl->templateSpec, parentDeclType, argumentsToApply);
		if (eval.ev.progress == symbol_component::EvaluationProgress::Evaluated)
		{
			if (eval.ev.ExtraCount() != 0 || classDecl->baseTypes.Count() == 0)
			{
				return eval.ev;
			}
		}

		eval.ev.progress = symbol_component::EvaluationProgress::Evaluating;
		eval.ev.AllocateExtra(classDecl->baseTypes.Count());

		for (vint i = 0; i < classDecl->baseTypes.Count(); i++)
		{
			TypeToTsysNoVta(eval.declPa, classDecl->baseTypes[i].f1, eval.ev.GetExtra(i));
		}

		eval.ev.progress = symbol_component::EvaluationProgress::Evaluated;
		return eval.ev;
	}

	symbol_component::Evaluation& EvaluateClassType(const ParsingArguments& invokerPa, ITsys* classType)
	{
		switch (classType->GetType())
		{
		case TsysType::Decl:
		case TsysType::DeclInstant:
			{
				auto symbol = classType->GetDecl();
				auto classDecl = symbol->GetImplDecl_NFb<ClassDeclaration>();
				if (!classDecl) throw TypeCheckerException();

				if (classType->GetType() == TsysType::Decl)
				{
					return EvaluateClassSymbol(invokerPa, classDecl.Obj(), nullptr, nullptr);
				}
				else
				{
					const auto& di = classType->GetDeclInstant();
					return EvaluateClassSymbol(invokerPa, classDecl.Obj(), di.parentDeclType, di.taContext.Obj());
				}
			}
		default:
			throw TypeCheckerException();
		}
	}
}