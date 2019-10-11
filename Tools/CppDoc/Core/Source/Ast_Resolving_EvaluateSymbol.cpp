#include "Ast_Resolving.h"

namespace symbol_type_resolving
{
	symbol_component::Evaluation& GetCorrectEvaluation(const ParsingArguments& pa, Declaration* decl, Ptr<TemplateSpec> spec, EvaluateSymbolContext* esContext)
	{
		switch (pa.GetEvaluationKind(decl, spec))
		{
		case EvaluationKind::General:
			return decl->symbol->GetEvaluationForUpdating_NFb();
		case EvaluationKind::Instantiated:
			return esContext->evaluation;
		default:
			{
				auto taContext = pa.taContext;
				while (taContext)
				{
					vint index = taContext->symbolEvaluations.Keys().IndexOf(decl->symbol);
					if (index != -1)
					{
						return *taContext->symbolEvaluations.Values()[index].Obj();
					}
					taContext = taContext->parent;
				}

				taContext = pa.taContext;
				auto ev = MakePtr<symbol_component::Evaluation>();
				taContext->symbolEvaluations.Add(decl->symbol, ev);
				return *ev.Obj();
			}
		}
	}

	ParsingArguments GetPaFromInvokerPa(const ParsingArguments& pa, Symbol* declSymbol, TemplateArgumentContext* parentTaContext, EvaluateSymbolContext* esContext)
	{
		auto newPa = pa.AdjustForDecl(declSymbol);
		if (parentTaContext && esContext)
		{
			esContext->additionalArguments.parent = parentTaContext;
		}

		if (esContext)
		{
			return newPa.WithArgs(esContext->additionalArguments);
		}
		else if (parentTaContext)
		{
			return newPa.WithArgs(*parentTaContext);
		}
		else
		{
			return newPa;
		}
	}

	TypeTsysList& FinishEvaluatingPotentialGenericSymbol(const ParsingArguments& declPa, Declaration* decl, Ptr<TemplateSpec> spec, EvaluateSymbolContext* esContext)
	{
		auto& ev = GetCorrectEvaluation(declPa, decl, spec, esContext);
		auto& evaluatedTypes = ev.Get();

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

		ev.progress = symbol_component::EvaluationProgress::Evaluated;
		return evaluatedTypes;
	}

#define SETUP_EV_OR_RETURN(DECL, SPEC, PARENTTACONTEXT, ESCONTEXT)											\
	auto symbol = DECL->symbol;																				\
	auto declPa = GetPaFromInvokerPa(invokerPa, DECL->symbol, PARENTTACONTEXT, ESCONTEXT);					\
	auto& ev = GetCorrectEvaluation(declPa, DECL, SPEC, ESCONTEXT);											\
	switch (ev.progress)																					\
	{																										\
	case symbol_component::EvaluationProgress::Evaluated: return ev.Get();									\
	case symbol_component::EvaluationProgress::Evaluating: throw NotResolvableException();					\
	}																										\
																											\
	ev.progress = symbol_component::EvaluationProgress::Evaluating;											\
	ev.Allocate();																							\
	auto& evaluatedTypes = ev.Get()																			\

	/***********************************************************************
	EvaluateVarSymbol: Evaluate the declared type for a variable
	***********************************************************************/

	TypeTsysList& EvaluateVarSymbol(const ParsingArguments& invokerPa, ForwardVariableDeclaration* varDecl)
	{
		SETUP_EV_OR_RETURN(varDecl, nullptr, nullptr, nullptr);

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

		ev.progress = symbol_component::EvaluationProgress::Evaluated;
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
		auto& ev = GetCorrectEvaluation(pa, funcDecl, funcDecl->templateSpec, esContext);
		if (ev.progress != symbol_component::EvaluationProgress::Evaluating)
		{
			throw NotResolvableException();
		}

		auto newPa = pa.WithScope(symbol->GetParentScope());
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

		FinishEvaluatingPotentialGenericSymbol(pa, funcDecl, funcDecl->templateSpec, esContext);
	}

	TypeTsysList& EvaluateFuncSymbol(const ParsingArguments& invokerPa, ForwardFunctionDeclaration* funcDecl, EvaluateSymbolContext* esContext)
	{
		SETUP_EV_OR_RETURN(funcDecl, funcDecl->templateSpec, nullptr, esContext);

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

	TemplateArgumentContext* GetTaContextFromDeclInstance(ITsys* parentDeclType)
	{
		if (!parentDeclType) return nullptr;
		if (parentDeclType->GetType() != TsysType::DeclInstant)
		{
			throw "Wrong parentDeclType.";
		}

		const auto& data = parentDeclType->GetDeclInstant();
		if (!data.taContext)
		{
			throw "Wrong parentDeclType";
		}

		return data.taContext.Obj();
	}

	TypeTsysList& EvaluateForwardClassSymbol(const ParsingArguments& invokerPa, ForwardClassDeclaration* classDecl, ITsys* parentDeclType, EvaluateSymbolContext* esContext)
	{
		if (esContext)
		{
			// not implemented
			throw 0;
		}
		auto parentTaContext = GetTaContextFromDeclInstance(parentDeclType);
		SETUP_EV_OR_RETURN(classDecl, classDecl->templateSpec, parentTaContext, esContext);

		ITsys* parentTemplateClass = nullptr;
		{
			auto parent = symbol->GetParentScope();
			while (parent)
			{
				switch (parent->kind)
				{
				case symbol_component::SymbolKind::Class:
				case symbol_component::SymbolKind::Struct:
				case symbol_component::SymbolKind::Union:
					if (auto parentClassDecl = parent->GetAnyForwardDecl<ForwardClassDeclaration>())
					{
						if (parentClassDecl->templateSpec)
						{
							parentTemplateClass = EvaluateForwardClassSymbol(invokerPa, parentClassDecl.Obj())[0]->GetElement();
							goto FINISH_PARENT_TEMPLATE;
						}
					}
					break;
				}
				parent = parent->GetParentScope();
			}
		}
	FINISH_PARENT_TEMPLATE:

		if (parentTemplateClass)
		{
			if (parentDeclType)
			{
				if (parentTemplateClass->GetDecl() != parentDeclType->GetDecl())
				{
					throw L"parentDeclType does not point to an instance of the inner most parent template class of classDecl.";
				}
				parentTemplateClass = parentDeclType;
			}
		}
		else if (parentDeclType)
		{
			throw L"parentDeclType should be nullptr if classDecl is not contained by a template class.";
		}

		if (classDecl->templateSpec)
		{
			Array<ITsys*> params(classDecl->templateSpec->arguments.Count());
			for (vint i = 0; i < classDecl->templateSpec->arguments.Count(); i++)
			{
				params[i] = GetTemplateArgumentKey(classDecl->templateSpec->arguments[i], declPa.tsys.Obj());
			}
			auto diTsys = declPa.tsys->DeclInstantOf(symbol, &params, parentTemplateClass);
			evaluatedTypes.Add(diTsys);
		}
		else
		{
			if (parentTemplateClass)
			{
				evaluatedTypes.Add(declPa.tsys->DeclInstantOf(symbol, nullptr, parentTemplateClass));
			}
			else
			{
				evaluatedTypes.Add(declPa.tsys->DeclOf(symbol));
			}
		}
		return FinishEvaluatingPotentialGenericSymbol(declPa, classDecl, classDecl->templateSpec, esContext);
	}

	symbol_component::Evaluation& EvaluateClassSymbol(const ParsingArguments& invokerPa, ClassDeclaration* classDecl, ITsys* parentDeclType, EvaluateSymbolContext* esContext)
	{
		if (esContext)
		{
			// not implemented
			throw 0;
		}
		auto parentTaContext = GetTaContextFromDeclInstance(parentDeclType);
		EvaluateForwardClassSymbol(invokerPa, classDecl, parentDeclType, esContext);

		auto symbol = classDecl->symbol;
		auto declPa = GetPaFromInvokerPa(invokerPa, classDecl->symbol, parentTaContext, esContext);
		auto& ev = GetCorrectEvaluation(declPa, classDecl, classDecl->templateSpec, esContext);
		switch (ev.progress)
		{
		case symbol_component::EvaluationProgress::Evaluated:
			if (ev.ExtraCount() == 0 && classDecl->baseTypes.Count() > 0)
			{
				break;
			}
			else
			{
				return ev;
			}
		case symbol_component::EvaluationProgress::Evaluating:
			throw NotResolvableException();
		}

		ev.progress = symbol_component::EvaluationProgress::Evaluating;
		ev.AllocateExtra(classDecl->baseTypes.Count());

		for (vint i = 0; i < classDecl->baseTypes.Count(); i++)
		{
			TypeToTsysNoVta(declPa, classDecl->baseTypes[i].f1, ev.GetExtra(i));
		}

		ev.progress = symbol_component::EvaluationProgress::Evaluated;
		return ev;
	}

	/***********************************************************************
	EvaluateTypeAliasSymbol: Evaluate the declared type for an alias
	***********************************************************************/

	TypeTsysList& EvaluateTypeAliasSymbol(const ParsingArguments& invokerPa, TypeAliasDeclaration* usingDecl, EvaluateSymbolContext* esContext)
	{
		SETUP_EV_OR_RETURN(usingDecl, usingDecl->templateSpec, nullptr, esContext);

		TypeToTsysNoVta(declPa, usingDecl->type, evaluatedTypes);
		return FinishEvaluatingPotentialGenericSymbol(declPa, usingDecl, usingDecl->templateSpec, esContext);
	}

	/***********************************************************************
	EvaluateValueAliasSymbol: Evaluate the declared value for an alias
	***********************************************************************/

	TypeTsysList& EvaluateValueAliasSymbol(const ParsingArguments& invokerPa, ValueAliasDeclaration* usingDecl, EvaluateSymbolContext* esContext)
	{
		SETUP_EV_OR_RETURN(usingDecl, usingDecl->templateSpec, nullptr, esContext);

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