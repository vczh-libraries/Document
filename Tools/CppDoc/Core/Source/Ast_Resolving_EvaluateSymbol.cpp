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

	TypeTsysList& FinishEvaluatingPotentialGenericSymbol(const ParsingArguments& pa, Declaration* decl, Ptr<TemplateSpec> spec, EvaluateSymbolContext* esContext)
	{
		auto& evaluatedTypes = GetEvaluatingResultStorage(pa, decl, spec, esContext);

		if (spec && !esContext)
		{
			TsysGenericFunction genericFunction;
			TypeTsysList params;

			genericFunction.declSymbol = decl->symbol;
			CreateGenericFunctionHeader(pa, spec, params, genericFunction);

			for (vint i = 0; i < evaluatedTypes.Count(); i++)
			{
				evaluatedTypes[i] = evaluatedTypes[i]->GenericFunctionOf(params, genericFunction);
			}
		}

		if (!esContext)
		{
			auto& ev = decl->symbol->GetEvaluationForUpdating_NFb();
			ev.progress = symbol_component::EvaluationProgress::Evaluated;
		}
		return evaluatedTypes;
	}

#define SETUP_EV_OR_RETURN																					\
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
	}

	/***********************************************************************
	EvaluateVarSymbol: Evaluate the declared type for a variable
	***********************************************************************/

	TypeTsysList& EvaluateVarSymbol(const ParsingArguments& invokerPa, ForwardVariableDeclaration* varDecl)
	{
		auto symbol = varDecl->symbol;
		SETUP_EV_OR_RETURN;

		auto pa = GetPaFromInvokerPa(invokerPa, varDecl->symbol, nullptr);
		auto newPa = pa.WithScope(symbol->GetParentScope());
		auto& evaluatedTypes = GetEvaluatingResultStorage(pa, varDecl, nullptr, nullptr);

		if (varDecl->needResolveTypeFromInitializer)
		{
			if (auto rootVarDecl = dynamic_cast<VariableDeclaration*>(varDecl))
			{
				if (!rootVarDecl->initializer)
				{
					throw NotResolvableException();
				}

				ExprTsysList types;
				ExprToTsysNoVta(newPa, rootVarDecl->initializer->arguments[0].item, types);

				for (vint k = 0; k < types.Count(); k++)
				{
					auto type = ResolvePendingType(pa, rootVarDecl->type, types[k]);
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
			TypeToTsysNoVta(newPa, varDecl->type, evaluatedTypes);
		}

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
		if(!esContext)
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
		auto symbol = funcDecl->symbol;
		if (!esContext) SETUP_EV_OR_RETURN;

		auto newPa = GetPaFromInvokerPa(invokerPa, symbol, esContext);
		auto& evaluatedTypes = GetEvaluatingResultStorage(newPa, funcDecl, funcDecl->templateSpec, esContext);

		if (funcDecl->needResolveTypeFromStatement)
		{
			if (auto rootFuncDecl = dynamic_cast<FunctionDeclaration*>(funcDecl))
			{
				EnsureFunctionBodyParsed(rootFuncDecl);
				EvaluateStat(newPa, rootFuncDecl->statement, true, esContext);

				if (evaluatedTypes.Count() == 0)
				{
					evaluatedTypes.Add(newPa.tsys->Void());
					return FinishEvaluatingPotentialGenericSymbol(newPa, funcDecl, funcDecl->templateSpec, esContext);
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
			TypeToTsysNoVta(newPa.WithScope(symbol->GetParentScope()), funcDecl->type, evaluatedTypes, IsMemberFunction(funcDecl));
			return FinishEvaluatingPotentialGenericSymbol(newPa, funcDecl, funcDecl->templateSpec, esContext);
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
		auto symbol = usingDecl->symbol;
		if (!esContext) SETUP_EV_OR_RETURN;

		auto newPa = GetPaFromInvokerPa(invokerPa, symbol, esContext)
			.WithScope(symbol->GetParentScope());
		auto& evaluatedTypes = GetEvaluatingResultStorage(newPa, usingDecl, usingDecl->templateSpec, esContext);

		TypeToTsysNoVta(newPa, usingDecl->type, evaluatedTypes);
		return FinishEvaluatingPotentialGenericSymbol(newPa, usingDecl, usingDecl->templateSpec, esContext);
	}

	/***********************************************************************
	EvaluateValueAliasSymbol: Evaluate the declared value for an alias
	***********************************************************************/

	TypeTsysList& EvaluateValueAliasSymbol(const ParsingArguments& invokerPa, ValueAliasDeclaration* usingDecl, EvaluateSymbolContext* esContext)
	{
		auto symbol = usingDecl->symbol;
		if (!esContext) SETUP_EV_OR_RETURN;

		auto newPa = GetPaFromInvokerPa(invokerPa, symbol, esContext)
			.WithScope(symbol->GetParentScope());
		auto& evaluatedTypes = GetEvaluatingResultStorage(newPa, usingDecl, usingDecl->templateSpec, esContext);

		if (usingDecl->needResolveTypeFromInitializer)
		{
			ExprTsysList tsys;
			ExprToTsysNoVta(newPa, usingDecl->expr, tsys);
			for (vint i = 0; i < tsys.Count(); i++)
			{
				auto entityType = tsys[i].tsys;
				if (entityType->GetType() == TsysType::Zero)
				{
					entityType = newPa.tsys->Int();
				}
				if (!evaluatedTypes.Contains(entityType))
				{
					evaluatedTypes.Add(entityType);
				}
			}
		}
		else
		{
			TypeToTsysNoVta(newPa, usingDecl->type, evaluatedTypes);
		}

		return FinishEvaluatingPotentialGenericSymbol(newPa, usingDecl, usingDecl->templateSpec, esContext);
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