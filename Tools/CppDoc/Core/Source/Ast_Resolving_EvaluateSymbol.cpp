#include "Ast_Resolving.h"

namespace symbol_type_resolving
{
	/***********************************************************************
	EvaluateVarSymbol: Evaluate the declared type for a variable
	***********************************************************************/

	TypeTsysList& EvaluateVarSymbol(const ParsingArguments& pa, ForwardVariableDeclaration* varDecl)
	{
		auto symbol = varDecl->symbol;
		auto& ev = symbol->GetEvaluationForUpdating_NFb();
		switch (ev.progress)
		{
		case symbol_component::EvaluationProgress::Evaluated: return ev.Get();
		case symbol_component::EvaluationProgress::Evaluating: throw NotResolvableException();
		}

		ev.progress = symbol_component::EvaluationProgress::Evaluating;
		ev.Allocate();

		auto newPa = pa.WithContext(symbol->GetParentScope());

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
					if (!ev.Get().Contains(type))
					{
						ev.Get().Add(type);
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
			TypeToTsysNoVta(newPa, varDecl->type, ev.Get(), nullptr);
		}
		ev.progress = symbol_component::EvaluationProgress::Evaluated;
		return ev.Get();
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

	void FinishEvaluatingSymbol(const ParsingArguments& pa, FunctionDeclaration* funcDecl)
	{
		auto symbol = funcDecl->symbol;
		auto& ev = symbol->GetEvaluationForUpdating_NFb();
		auto newPa = pa.WithContext(symbol->GetParentScope());
		TypeTsysList returnTypes;
		CopyFrom(returnTypes, ev.Get());
		ev.Get().Clear();

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
		TypeToTsysAndReplaceFunctionReturnType(newPa, funcDecl->type, processedReturnTypes, ev.Get(), nullptr, IsMemberFunction(pa, funcDecl));
		ev.progress = symbol_component::EvaluationProgress::Evaluated;
	}

	TypeTsysList& EvaluateFuncSymbol(const ParsingArguments& pa, ForwardFunctionDeclaration* funcDecl, EvaluateSymbolContext* esContext)
	{
		auto symbol = funcDecl->symbol;
		auto& ev = symbol->GetEvaluationForUpdating_NFb();
		switch (ev.progress)
		{
		case symbol_component::EvaluationProgress::Evaluated: return ev.Get();
		case symbol_component::EvaluationProgress::Evaluating: throw NotResolvableException();
		}

		ev.progress = symbol_component::EvaluationProgress::Evaluating;
		
		if (funcDecl->needResolveTypeFromStatement)
		{
			if (auto rootFuncDecl = dynamic_cast<FunctionDeclaration*>(funcDecl))
			{
				EnsureFunctionBodyParsed(rootFuncDecl);
				auto funcPa = pa.WithContext(symbol);
				EvaluateStat(funcPa, rootFuncDecl->statement);
				if (ev.Count() == 0)
				{
					throw NotResolvableException();
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
			ev.Allocate();
			TypeToTsysNoVta(newPa, funcDecl->type, ev.Get(), nullptr, IsMemberFunction(pa, funcDecl));
			ev.progress = symbol_component::EvaluationProgress::Evaluated;
		}

		if (funcDecl->templateSpec && !esContext)
		{
			TsysGenericFunction genericFunction;
			TypeTsysList params;

			genericFunction.declSymbol = symbol;
			CreateGenericFunctionHeader(pa, funcDecl->templateSpec, params, genericFunction);

			for (vint i = 0; i < ev.Get().Count(); i++)
			{
				ev.Get()[i] = ev.Get()[i]->GenericFunctionOf(params, genericFunction);
			}
		}
		return ev.Get();
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

		{
			auto newPa = pa.WithContext(symbol);
			for (vint i = 0; i < classDecl->baseTypes.Count(); i++)
			{
				TypeToTsysNoVta(newPa, classDecl->baseTypes[i].f1, ev.Get(i), nullptr);
			}
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
		if (!esContext)
		{
			auto& ev = symbol->GetEvaluationForUpdating_NFb();
			switch (ev.progress)
			{
			case symbol_component::EvaluationProgress::Evaluated: return ev.Get();
			case symbol_component::EvaluationProgress::Evaluating: throw NotResolvableException();
			}
			ev.progress = symbol_component::EvaluationProgress::Evaluating;
			ev.Allocate();
		}

		auto newPa = pa.WithContext(symbol->GetParentScope());
		auto& evaluatedTypes = esContext ? esContext->evaluatedTypes : symbol->GetEvaluationForUpdating_NFb().Get();

		TypeTsysList types;
		TypeToTsysNoVta(newPa, usingDecl->type, types, (esContext ? &esContext->gaContext : nullptr));

		if (usingDecl->templateSpec && !esContext)
		{
			TsysGenericFunction genericFunction;
			TypeTsysList params;

			genericFunction.declSymbol = symbol;
			CreateGenericFunctionHeader(pa, usingDecl->templateSpec, params, genericFunction);

			for (vint i = 0; i < types.Count(); i++)
			{
				evaluatedTypes.Add(types[i]->GenericFunctionOf(params, genericFunction));
			}
		}
		else
		{
			CopyFrom(evaluatedTypes, types);
		}

		if (!esContext)
		{
			auto& ev = symbol->GetEvaluationForUpdating_NFb();
			ev.progress = symbol_component::EvaluationProgress::Evaluated;
		}
		return evaluatedTypes;
	}

	/***********************************************************************
	EvaluateValueAliasSymbol: Evaluate the declared value for an alias
	***********************************************************************/

	TypeTsysList& EvaluateValueAliasSymbol(const ParsingArguments& pa, ValueAliasDeclaration* usingDecl, EvaluateSymbolContext* esContext)
	{
		auto symbol = usingDecl->symbol;
		if (!esContext)
		{
			auto& ev = symbol->GetEvaluationForUpdating_NFb();
			switch (ev.progress)
			{
			case symbol_component::EvaluationProgress::Evaluated: return ev.Get();
			case symbol_component::EvaluationProgress::Evaluating: throw NotResolvableException();
			}
			ev.progress = symbol_component::EvaluationProgress::Evaluating;
			ev.Allocate();
		}

		auto newPa = pa.WithContext(symbol->GetParentScope());
		auto& evaluatedTypes = esContext ? esContext->evaluatedTypes : symbol->GetEvaluationForUpdating_NFb().Get();

		TypeTsysList types;
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
				if (!types.Contains(entityType))
				{
					types.Add(entityType);
				}
			}
		}
		else
		{
			TypeToTsysNoVta(newPa, usingDecl->type, types, (esContext ? &esContext->gaContext : nullptr));
		}

		if (usingDecl->templateSpec && !esContext)
		{
			TsysGenericFunction genericFunction;
			TypeTsysList params;

			genericFunction.declSymbol = symbol;
			CreateGenericFunctionHeader(pa, usingDecl->templateSpec, params, genericFunction);

			for (vint i = 0; i < types.Count(); i++)
			{
				evaluatedTypes.Add(types[i]->GenericFunctionOf(params, genericFunction));
			}
		}
		else
		{
			CopyFrom(evaluatedTypes, types);
		}

		if (!esContext)
		{
			auto& ev = symbol->GetEvaluationForUpdating_NFb();
			ev.progress = symbol_component::EvaluationProgress::Evaluated;
		}
		return evaluatedTypes;
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