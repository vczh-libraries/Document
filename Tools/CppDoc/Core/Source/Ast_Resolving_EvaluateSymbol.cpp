#include "Ast_Resolving.h"

namespace symbol_type_resolving
{
	TypeTsysList& GetEvaluatingResultStorage(Symbol* symbol, EvaluateSymbolContext* esContext)
	{
		return esContext ? esContext->evaluatedTypes : symbol->GetEvaluationForUpdating_NFb().Get();
	}

	TypeTsysList& FinishEvaluatingPotentialGenericSymbol(const ParsingArguments& pa, Symbol* symbol, Ptr<TemplateSpec> spec, EvaluateSymbolContext* esContext)
	{
		auto& evaluatedTypes = GetEvaluatingResultStorage(symbol, esContext);

		if (spec && !esContext)
		{
			TsysGenericFunction genericFunction;
			TypeTsysList params;

			genericFunction.declSymbol = symbol;
			CreateGenericFunctionHeader(pa, spec, params, genericFunction);

			for (vint i = 0; i < evaluatedTypes.Count(); i++)
			{
				evaluatedTypes[i] = evaluatedTypes[i]->GenericFunctionOf(params, genericFunction);
			}
		}

		if (!esContext)
		{
			auto& ev = symbol->GetEvaluationForUpdating_NFb();
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

	TypeTsysList& EvaluateVarSymbol(const ParsingArguments& pa, ForwardVariableDeclaration* varDecl)
	{
		auto symbol = varDecl->symbol;
		SETUP_EV_OR_RETURN;

		auto newPa = pa.WithContext(symbol->GetParentScope());
		auto& evaluatedTypes = GetEvaluatingResultStorage(symbol, nullptr);

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
			TypeToTsysNoVta(newPa, varDecl->type, evaluatedTypes, nullptr);
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

	bool IsMemberFunction(const ParsingArguments& pa, ForwardFunctionDeclaration* funcDecl)
	{
		auto symbol = funcDecl->symbol;
		ITsys* classScope = nullptr;
		if (auto parent = symbol->GetParentScope())
		{
			if (parent->GetImplDecl_NFb<ClassDeclaration>())
			{
				classScope = pa.tsys->DeclOf(parent);
			}
		}

		bool isStaticSymbol = IsStaticSymbol<ForwardFunctionDeclaration>(symbol);
		return classScope && !isStaticSymbol;
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

		auto newPa = pa.WithContext(symbol->GetParentScope());
		auto& evaluatedTypes = GetEvaluatingResultStorage(symbol, esContext);

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
			(esContext ? &esContext->gaContext : nullptr),
			IsMemberFunction(pa, funcDecl)
		);

		FinishEvaluatingPotentialGenericSymbol(pa, symbol, funcDecl->templateSpec, esContext);
	}

	TypeTsysList& EvaluateFuncSymbol(const ParsingArguments& pa, ForwardFunctionDeclaration* funcDecl, EvaluateSymbolContext* esContext)
	{
		auto symbol = funcDecl->symbol;
		if (!esContext) SETUP_EV_OR_RETURN;

		auto& evaluatedTypes = GetEvaluatingResultStorage(symbol, esContext);

		if (funcDecl->needResolveTypeFromStatement)
		{
			if (auto rootFuncDecl = dynamic_cast<FunctionDeclaration*>(funcDecl))
			{
				EnsureFunctionBodyParsed(rootFuncDecl);
				auto funcPa = pa.WithContext(symbol);
				EvaluateStat(funcPa, rootFuncDecl->statement, true, esContext);

				if (evaluatedTypes.Count() == 0)
				{
					evaluatedTypes.Add(pa.tsys->Void());
					return FinishEvaluatingPotentialGenericSymbol(pa, symbol, funcDecl->templateSpec, esContext);
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
			auto newPa = pa.WithContext(symbol->GetParentScope());
			TypeToTsysNoVta(newPa, funcDecl->type, evaluatedTypes, nullptr, IsMemberFunction(pa, funcDecl));
			return FinishEvaluatingPotentialGenericSymbol(pa, symbol, funcDecl->templateSpec, esContext);
		}
	}

	/***********************************************************************
	EvaluateClassSymbol: Evaluate base types for a class
	***********************************************************************/

	symbol_component::Evaluation& EvaluateClassSymbol(const ParsingArguments& pa, ClassDeclaration* classDecl)
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

		auto newPa = pa.WithContext(symbol);
		for (vint i = 0; i < classDecl->baseTypes.Count(); i++)
		{
			TypeToTsysNoVta(newPa, classDecl->baseTypes[i].f1, ev.Get(i), nullptr);
		}

		ev.progress = symbol_component::EvaluationProgress::Evaluated;
		return ev;
	}

	/***********************************************************************
	EvaluateTypeAliasSymbol: Evaluate the declared type for an alias
	***********************************************************************/

	TypeTsysList& EvaluateTypeAliasSymbol(const ParsingArguments& pa, TypeAliasDeclaration* usingDecl, EvaluateSymbolContext* esContext)
	{
		auto symbol = usingDecl->symbol;
		if (!esContext) SETUP_EV_OR_RETURN;

		auto newPa = pa.WithContext(symbol->GetParentScope());
		auto& evaluatedTypes = GetEvaluatingResultStorage(symbol, esContext);

		TypeToTsysNoVta(newPa, usingDecl->type, evaluatedTypes, (esContext ? &esContext->gaContext : nullptr));
		return FinishEvaluatingPotentialGenericSymbol(pa, symbol, usingDecl->templateSpec, esContext);
	}

	/***********************************************************************
	EvaluateValueAliasSymbol: Evaluate the declared value for an alias
	***********************************************************************/

	TypeTsysList& EvaluateValueAliasSymbol(const ParsingArguments& pa, ValueAliasDeclaration* usingDecl, EvaluateSymbolContext* esContext)
	{
		auto symbol = usingDecl->symbol;
		if (!esContext) SETUP_EV_OR_RETURN;

		auto newPa = pa.WithContext(symbol->GetParentScope());
		auto& evaluatedTypes = GetEvaluatingResultStorage(symbol, esContext);

		if (usingDecl->needResolveTypeFromInitializer)
		{
			ExprTsysList tsys;
			ExprToTsysNoVta(newPa, usingDecl->expr, tsys, (esContext ? &esContext->gaContext : nullptr));
			for (vint i = 0; i < tsys.Count(); i++)
			{
				auto entityType = tsys[i].tsys;
				if (entityType->GetType() == TsysType::Zero)
				{
					entityType = pa.tsys->Int();
				}
				if (!evaluatedTypes.Contains(entityType))
				{
					evaluatedTypes.Add(entityType);
				}
			}
		}
		else
		{
			TypeToTsysNoVta(newPa, usingDecl->type, evaluatedTypes, (esContext ? &esContext->gaContext : nullptr));
		}

		return FinishEvaluatingPotentialGenericSymbol(pa, symbol, usingDecl->templateSpec, esContext);
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