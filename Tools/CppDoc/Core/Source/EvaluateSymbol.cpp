#include "EvaluateSymbol_Shared.h"
#include "Symbol_TemplateSpec.h"
#include "Ast.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	GetCorrectEvaluation: Get the correct place to cache the evaluation result
	***********************************************************************/

	symbol_component::Evaluation& GetCorrectEvaluation(const ParsingArguments& pa, Declaration* decl, Ptr<TemplateSpec> spec, TemplateArgumentContext* argumentsToApply)
	{
		switch (pa.GetEvaluationKind(decl, spec))
		{
		case EvaluationKind::General:
			return decl->symbol->GetEvaluationForUpdating_NFb();
		case EvaluationKind::Instantiated:
			{
				if (!spec)
				{
					throw L"Missing TemplateSpec for EvaluationKind::Instantiated";
				}
				if (!argumentsToApply)
				{
					throw L"Missing TemplateArgumentContext for EvaluationKind::Instantiated";
				}

				symbol_component::SG_Cache cacheKey;
				cacheKey.parentDeclTypeAndParams = MakePtr<Array<ITsys*>>(spec->arguments.Count() + 1);
				cacheKey.parentDeclTypeAndParams->Set(0, pa.parentDeclType);
				for (vint i = 0; i < spec->arguments.Count(); i++)
				{
					auto key = GetTemplateArgumentKey(spec->arguments[i], pa.tsys.Obj());
					cacheKey.parentDeclTypeAndParams->Set(i + 1, argumentsToApply->arguments[key]);
				}

				vint index = decl->symbol->genericCaches.IndexOf(cacheKey);
				if (index != -1)
				{
					return *decl->symbol->genericCaches[index].cachedEvaluation.Obj();
				}

				cacheKey.cachedEvaluation = MakePtr<symbol_component::Evaluation>();
				decl->symbol->genericCaches.Add(cacheKey);
				return *cacheKey.cachedEvaluation.Obj();
			}
		default:
			{
				// when evaluating local symbols for an uninitialized function, pa.taContext could belong to the template class
				// in this case, we need to stop at the class when the function is initialized
				// so that it knows that the symbol is not evaluated, instead of returning the evaluated symbol for the unitialized function
				auto taContext = pa.taContext;
				auto topTaContextToExamine = ParsingArguments::AdjustTaContextForScope(decl->symbol, taContext);

				while (taContext)
				{
					vint index = taContext->symbolEvaluations.Keys().IndexOf(decl->symbol);
					if (index != -1)
					{
						return *taContext->symbolEvaluations.Values()[index].Obj();
					}
					if (taContext == topTaContextToExamine) break;
					taContext = taContext->parent;
				}

				taContext = pa.taContext;
				auto ev = MakePtr<symbol_component::Evaluation>();
				taContext->symbolEvaluations.Add(decl->symbol, ev);
				return *ev.Obj();
			}
		}
	}

	/***********************************************************************
	GetPaFromInvokerPa: Adjust the context to evaluate the content inside an instantiated template declaration
	***********************************************************************/

	ParsingArguments GetPaFromInvokerPa(const ParsingArguments& pa, Symbol* declSymbol, TemplateArgumentContext* parentTaContext, TemplateArgumentContext* argumentsToApply)
	{
		auto newPa = pa.AdjustForDecl(declSymbol);
		if (argumentsToApply)
		{
			if (parentTaContext)
			{
				argumentsToApply->parent = parentTaContext;
			}
			newPa.taContext = argumentsToApply;
			return newPa;
		}
		else
		{
			// esContext == nullptr means that it is not requested by GenericExpr or GenericType
			// so we must adjust taContext to it's parent symbol
			
			if (parentTaContext)
			{
				// parentDeclType is provided, so just use it
				newPa.taContext = parentTaContext;
			}
			else
			{
				// parentDeclType is not provided, need to check taContext
				if (auto ta = ParsingArguments::AdjustTaContextForScope(declSymbol, newPa.taContext))
				{
					if (ta->symbolToApply == declSymbol)
					{
						// for all taContext that after this ta
						// those type arguments are useless because they replace internal symbols
						// so we use this ta's parent
						// in this case this symbol will be evaluated to a GenericFunction if it has TemplateSpec
						// this is for ensuring GetEvaluationKind doesn't return EvaluationKind::Instantiated
						newPa.taContext = ta->parent;
					}
				}
			}
			return newPa;
		}
	}

	/***********************************************************************
	FinishEvaluatingPotentialGenericSymbol: Encapsulate the type with TsysGenericFunction if necessary
	***********************************************************************/

	TypeTsysList& FinishEvaluatingPotentialGenericSymbol(const ParsingArguments& declPa, Declaration* decl, Ptr<TemplateSpec> spec, TemplateArgumentContext* argumentsToApply)
	{
		auto& ev = GetCorrectEvaluation(declPa, decl, spec, argumentsToApply);
		auto& evaluatedTypes = ev.Get();

		if (spec && !argumentsToApply)
		{
			TsysGenericFunction genericFunction;
			TypeTsysList params;

			CreateGenericFunctionHeader(declPa, decl->symbol, declPa.parentDeclType, spec, params, genericFunction);

			for (vint i = 0; i < evaluatedTypes.Count(); i++)
			{
				evaluatedTypes[i] = evaluatedTypes[i]->GenericFunctionOf(params, genericFunction);
			}
		}

		ev.progress = symbol_component::EvaluationProgress::Evaluated;
		return evaluatedTypes;
	}

	/***********************************************************************
	ProcessArguments: Create the context to evaluate the content inside an instantiated template declaration
	***********************************************************************/

	Eval ProcessArguments(const ParsingArguments& invokerPa, Declaration* decl, Ptr<TemplateSpec> spec, ITsys*& parentDeclType, TemplateArgumentContext* argumentsToApply, bool allowEvaluating)
	{
		if (parentDeclType)
		{
			if (parentDeclType->GetType() != TsysType::DeclInstant)
			{
				throw L"parentDeclType should be DeclInstant";
			}

			const auto& data = parentDeclType->GetDeclInstant();
			if (!data.taContext)
			{
				parentDeclType = data.parentDeclType;
			}
		}

		TemplateArgumentContext* parentTaContext = nullptr;
		if (parentDeclType)
		{
			const auto& data = parentDeclType->GetDeclInstant();
			if (!data.taContext)
			{
				throw L"Wrong parentDeclType";
			}

			parentTaContext = data.taContext.Obj();
		}

		auto symbol = decl->symbol;
		Symbol* parentTemplateClassSymbol = nullptr;
		{
			auto parentClassSymbol = FindParentClassSymbol(symbol, false);
			while (parentClassSymbol)
			{
				if (auto parentClassDecl = parentClassSymbol->GetAnyForwardDecl<ForwardClassDeclaration>())
				{
					if (parentClassDecl->templateSpec)
					{
						parentTemplateClassSymbol = parentClassSymbol;
						goto FINISH_PARENT_TEMPLATE;
					}
				}
				parentClassSymbol = FindParentClassSymbol(parentClassSymbol, false);
			}
		}
	FINISH_PARENT_TEMPLATE:

		ITsys* parentTemplateClass = nullptr;
		if (parentTemplateClassSymbol)
		{
			if (parentDeclType)
			{
				if (parentTemplateClassSymbol != parentDeclType->GetDecl())
				{
					throw L"parentDeclType does not point to an instance of the inner most parent template class of classDecl.";
				}
				parentTemplateClass = parentDeclType;
			}
			else if (invokerPa.parentDeclType)
			{
				parentTemplateClass = ParsingArguments::AdjustDeclInstantForScope(parentTemplateClassSymbol, invokerPa.parentDeclType, true);
			}
			else
			{
				auto parentClassDecl = parentTemplateClassSymbol->GetAnyForwardDecl<ForwardClassDeclaration>();
				auto parentPa = invokerPa.AdjustForDecl(parentTemplateClassSymbol, nullptr, true);
				parentTemplateClass = EvaluateForwardClassSymbol(parentPa, parentClassDecl.Obj(), nullptr, nullptr)[0]->GetElement();
			}
		}
		else if (parentDeclType)
		{
			throw L"parentDeclType should be nullptr if classDecl is not contained by a template class.";
		}

		auto declPa = GetPaFromInvokerPa(invokerPa, symbol, parentTaContext, argumentsToApply);
		declPa.parentDeclType = parentTemplateClass;
		auto& ev = GetCorrectEvaluation(declPa, decl, spec, argumentsToApply);

		switch (ev.progress)
		{
		case symbol_component::EvaluationProgress::RecursiveFound:
		case symbol_component::EvaluationProgress::Evaluating:
			if (allowEvaluating)
			{
				ev.progress = symbol_component::EvaluationProgress::RecursiveFound;
				return Eval(true, symbol, declPa, ev);
			}
			else
			{
				throw TypeCheckerException();
			}
		case symbol_component::EvaluationProgress::Evaluated:
			return Eval(false, symbol, declPa, ev);
		default:
			ev.progress = symbol_component::EvaluationProgress::Evaluating;
			ev.Allocate();
			return Eval(true, symbol, declPa, ev);
		}
	}

	/***********************************************************************
	EvaluateGenericArgumentSymbol: Evaluate the declared value for a generic argument
	***********************************************************************/

	ITsys* EvaluateGenericArgumentSymbol(Symbol* symbol)
	{
		switch (symbol->kind)
		{
		case symbol_component::SymbolKind::GenericTypeArgument:
		case symbol_component::SymbolKind::GenericValueArgument:
			return symbol->GetEvaluationForUpdating_NFb().Get()[0];
		default:
			throw TypeCheckerException();
		}
	}
}