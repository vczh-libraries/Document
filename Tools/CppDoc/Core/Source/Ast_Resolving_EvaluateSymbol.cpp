#include "Ast_Resolving.h"

namespace symbol_type_resolving
{
	TypeTsysList& GetEvaluatingResultStorage(const ParsingArguments& pa, Declaration* decl, Ptr<TemplateSpec> spec, EvaluateSymbolContext* esContext)
	{
		switch (pa.GetEvaluationKind(decl, spec))
		{
		case EvaluationKind::General:
			return decl->symbol->GetEvaluationForUpdating_NFb().Get();
		case EvaluationKind::Instantiated:
			return esContext->evaluatedTypes;
		default:
			// TODO: GeneralUnderInstantiated will be stored in pa.taContext
			throw 0;
		}
	}

	ParsingArguments GetPaFromInvokerPa(const ParsingArguments& pa, Symbol* declSymbol, EvaluateSymbolContext* esContext)
	{
		auto newPa = pa.AdjustForDecl(declSymbol);
		return esContext ? newPa.WithArgs(esContext->additionalArguments) : newPa;
	}

	TypeTsysList& FinishEvaluatingPotentialGenericSymbol(const ParsingArguments& declPa, Declaration* decl, Ptr<TemplateSpec> spec, EvaluateSymbolContext* esContext)
	{
		auto& evaluatedTypes = GetEvaluatingResultStorage(declPa, decl, spec, esContext);

		if (spec && !esContext)
		{
			TsysGenericFunction genericFunction;
			TypeTsysList params;

			genericFunction.declSymbol = decl->symbol;
			CreateGenericFunctionHeader(declPa, spec, params, genericFunction);

			for (vint i = 0; i < evaluatedTypes.Count(); i++)
			{
				evaluatedTypes[i] = evaluatedTypes[i]->GenericFunctionOf(params, genericFunction);
			}
		}

		if (declPa.IsGeneralEvaluation())
		{
			auto& ev = decl->symbol->GetEvaluationForUpdating_NFb();
			ev.progress = symbol_component::EvaluationProgress::Evaluated;
		}
		return evaluatedTypes;
	}

#define SETUP_EV_OR_RETURN(DECL, SPEC, ESCONTEXT)															\
	auto symbol = DECL->symbol;																				\
	auto declPa = GetPaFromInvokerPa(invokerPa, DECL->symbol, ESCONTEXT);									\
	if (declPa.IsGeneralEvaluation())																		\
	{																										\
		auto& ev = symbol->GetEvaluationForUpdating_NFb();													\
		switch (ev.progress)																				\
		{																									\
		case symbol_component::EvaluationProgress::Evaluated: return ev.Get();								\
		case symbol_component::EvaluationProgress::Evaluating: throw NotResolvableException();				\
		}																									\
																											\
		ev.progress = symbol_component::EvaluationProgress::Evaluating;										\
		ev.Allocate();																						\
	}																										\
	auto& evaluatedTypes = GetEvaluatingResultStorage(declPa, DECL, SPEC, ESCONTEXT)						\

	/***********************************************************************
	EvaluateVarSymbol: Evaluate the declared type for a variable
	***********************************************************************/

	TypeTsysList& EvaluateVarSymbol(const ParsingArguments& invokerPa, ForwardVariableDeclaration* varDecl)
	{
		SETUP_EV_OR_RETURN(varDecl, nullptr, nullptr);

		if (varDecl->needResolveTypeFromInitializer)
		{
			if (auto rootVarDecl = dynamic_cast<VariableDeclaration*>(varDecl))
			{
				if (!rootVarDecl->initializer)
				{
					throw NotResolvableException();
				}

				ExprTsysList types;
				ExprToTsysNoVta(declPa, rootVarDecl->initializer->arguments[0].item, types);

				for (vint k = 0; k < types.Count(); k++)
				{
					auto type = ResolvePendingType(declPa, rootVarDecl->type, types[k]);
					if (!evaluatedTypes.Contains(type))
					{
						evaluatedTypes.Add(type);
					}
				}
			}
			else
			{
				throw NotResolvableException();
			}
		}
		else
		{
			TypeToTsysNoVta(declPa, varDecl->type, evaluatedTypes);
		}

		if (declPa.IsGeneralEvaluation())
		{
			auto& ev = symbol->GetEvaluationForUpdating_NFb();
			ev.progress = symbol_component::EvaluationProgress::Evaluated;
		}
		return evaluatedTypes;
	}

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

	void SetFuncTypeByReturnStat(const ParsingArguments& pa, FunctionDeclaration* funcDecl, TypeTsysList& returnTypes, EvaluateSymbolContext* esContext)
	{
		auto symbol = funcDecl->symbol;
		if(pa.IsGeneralEvaluation())
		{
			auto& ev = symbol->GetEvaluationForUpdating_NFb();
			if (ev.progress != symbol_component::EvaluationProgress::Evaluating)
			{
				throw NotResolvableException();
			}
		}

		auto newPa = pa.WithScope(symbol->GetParentScope());
		auto& evaluatedTypes = GetEvaluatingResultStorage(pa, funcDecl, funcDecl->templateSpec, esContext);

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

		FinishEvaluatingPotentialGenericSymbol(pa, funcDecl, funcDecl->templateSpec, esContext);
	}

	TypeTsysList& EvaluateFuncSymbol(const ParsingArguments& invokerPa, ForwardFunctionDeclaration* funcDecl, EvaluateSymbolContext* esContext)
	{
		SETUP_EV_OR_RETURN(funcDecl, funcDecl->templateSpec, esContext);

		if (funcDecl->needResolveTypeFromStatement)
		{
			if (auto rootFuncDecl = dynamic_cast<FunctionDeclaration*>(funcDecl))
			{
				EnsureFunctionBodyParsed(rootFuncDecl);
				EvaluateStat(declPa, rootFuncDecl->statement, true, esContext);

				if (evaluatedTypes.Count() == 0)
				{
					evaluatedTypes.Add(declPa.tsys->Void());
					return FinishEvaluatingPotentialGenericSymbol(declPa, funcDecl, funcDecl->templateSpec, esContext);
				}
				else
				{
					return evaluatedTypes;
				}
			}
			else
			{
				throw NotResolvableException();
			}
		}
		else
		{
			TypeToTsysNoVta(declPa.WithScope(symbol->GetParentScope()), funcDecl->type, evaluatedTypes, IsMemberFunction(funcDecl));
			return FinishEvaluatingPotentialGenericSymbol(declPa, funcDecl, funcDecl->templateSpec, esContext);
		}
	}

	/***********************************************************************
	EvaluateClassSymbol: Evaluate base types for a class
	***********************************************************************/

	symbol_component::Evaluation& EvaluateClassSymbol(const ParsingArguments& invokerPa, ClassDeclaration* classDecl)
	{
		auto symbol = classDecl->symbol;
		auto& ev = symbol->GetEvaluationForUpdating_NFb();
		switch (ev.progress)
		{
		case symbol_component::EvaluationProgress::Evaluated: return ev;
		case symbol_component::EvaluationProgress::Evaluating: throw NotResolvableException();
		}

		ev.progress = symbol_component::EvaluationProgress::Evaluating;
		ev.Allocate(classDecl->baseTypes.Count());

		auto newPa = GetPaFromInvokerPa(invokerPa, symbol, nullptr)
			.WithScope(symbol);
		for (vint i = 0; i < classDecl->baseTypes.Count(); i++)
		{
			TypeToTsysNoVta(newPa, classDecl->baseTypes[i].f1, ev.Get(i));
		}

		ev.progress = symbol_component::EvaluationProgress::Evaluated;
		return ev;
	}

	/***********************************************************************
	EvaluateTypeAliasSymbol: Evaluate the declared type for an alias
	***********************************************************************/

	TypeTsysList& EvaluateTypeAliasSymbol(const ParsingArguments& invokerPa, TypeAliasDeclaration* usingDecl, EvaluateSymbolContext* esContext)
	{
		SETUP_EV_OR_RETURN(usingDecl, usingDecl->templateSpec, esContext);

		TypeToTsysNoVta(declPa, usingDecl->type, evaluatedTypes);
		return FinishEvaluatingPotentialGenericSymbol(declPa, usingDecl, usingDecl->templateSpec, esContext);
	}

	/***********************************************************************
	EvaluateValueAliasSymbol: Evaluate the declared value for an alias
	***********************************************************************/

	TypeTsysList& EvaluateValueAliasSymbol(const ParsingArguments& invokerPa, ValueAliasDeclaration* usingDecl, EvaluateSymbolContext* esContext)
	{
		SETUP_EV_OR_RETURN(usingDecl, usingDecl->templateSpec, esContext);

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
				if (!evaluatedTypes.Contains(entityType))
				{
					evaluatedTypes.Add(entityType);
				}
			}
		}
		else
		{
			TypeToTsysNoVta(declPa, usingDecl->type, evaluatedTypes);
		}

		return FinishEvaluatingPotentialGenericSymbol(declPa, usingDecl, usingDecl->templateSpec, esContext);
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
			throw NotResolvableException();
		}
	}
}