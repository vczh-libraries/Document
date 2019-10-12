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
			if (!esContext)
			{
				throw L"Missing esContext for EvaluationKind::Instantiated";
			}
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
			newPa.taContext = &esContext->additionalArguments;
		}
		else if (parentTaContext)
		{
			newPa.taContext = parentTaContext;
		}
		return newPa;
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
			genericFunction.parentDeclType = declPa.parentDeclType;
			CreateGenericFunctionHeader(declPa, spec, params, genericFunction);

			for (vint i = 0; i < evaluatedTypes.Count(); i++)
			{
				evaluatedTypes[i] = evaluatedTypes[i]->GenericFunctionOf(params, genericFunction);
			}
		}

		ev.progress = symbol_component::EvaluationProgress::Evaluated;
		return evaluatedTypes;
	}

	struct Eval
	{
	private:
		bool								notEvaluated;

	public:
		Symbol*								symbol;
		ParsingArguments					declPa;
		symbol_component::Evaluation&		ev;
		TypeTsysList&						evaluatedTypes;

		Eval(
			bool							_notEvaluated,
			Symbol*							_symbol,
			ParsingArguments				_declPa,
			symbol_component::Evaluation&	_ev,
			TypeTsysList&					_evaluatedTypes
		)
			: notEvaluated(_notEvaluated)
			, symbol(_symbol)
			, declPa(_declPa)
			, ev(_ev)
			, evaluatedTypes(_evaluatedTypes)
		{
		}

		operator bool()
		{
			return notEvaluated;
		}
	};

	Eval ProcessArguments(const ParsingArguments& invokerPa, Declaration* decl, Ptr<TemplateSpec> spec, ITsys*& parentDeclType, EvaluateSymbolContext* esContext)
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
							parentTemplateClassSymbol = parent;
							goto FINISH_PARENT_TEMPLATE;
						}
					}
					break;
				}
				parent = parent->GetParentScope();
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

		auto declPa = GetPaFromInvokerPa(invokerPa, symbol, parentTaContext, esContext);
		declPa.parentDeclType = parentTemplateClass;
		auto& ev = GetCorrectEvaluation(declPa, decl, spec, esContext);

		switch (ev.progress)
		{
		case symbol_component::EvaluationProgress::Evaluating:
			throw NotResolvableException();
		case symbol_component::EvaluationProgress::Evaluated:
			return Eval(false, symbol, declPa, ev, ev.Get());
		default:
			ev.progress = symbol_component::EvaluationProgress::Evaluating;
			ev.Allocate();
			return Eval(true, symbol, declPa, ev, ev.Get());
		}
	}

	/***********************************************************************
	EvaluateVarSymbol: Evaluate the declared type for a variable
	***********************************************************************/

	TypeTsysList& EvaluateVarSymbol(
		const ParsingArguments& invokerPa,
		ForwardVariableDeclaration* varDecl,
		ITsys* parentDeclType
	)
	{
		auto eval = ProcessArguments(invokerPa, varDecl, nullptr, parentDeclType, nullptr);
		if(eval)
		{
			if (varDecl->needResolveTypeFromInitializer)
			{
				if (auto rootVarDecl = dynamic_cast<VariableDeclaration*>(varDecl))
				{
					if (!rootVarDecl->initializer)
					{
						throw NotResolvableException();
					}

					ExprTsysList types;
					ExprToTsysNoVta(eval.declPa, rootVarDecl->initializer->arguments[0].item, types);

					for (vint k = 0; k < types.Count(); k++)
					{
						auto type = ResolvePendingType(eval.declPa, rootVarDecl->type, types[k]);
						if (!eval.evaluatedTypes.Contains(type))
						{
							eval.evaluatedTypes.Add(type);
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
				TypeToTsysNoVta(eval.declPa, varDecl->type, eval.evaluatedTypes);
			}

			eval.ev.progress = symbol_component::EvaluationProgress::Evaluated;
			return eval.evaluatedTypes;
		}
		else
		{
			return eval.evaluatedTypes;
		}
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

	void SetFuncTypeByReturnStat(
		const ParsingArguments& pa,
		FunctionDeclaration* funcDecl,
		TypeTsysList& returnTypes,
		EvaluateSymbolContext* esContext
	)
	{
		auto symbol = funcDecl->symbol;
		auto& ev = GetCorrectEvaluation(pa, funcDecl, funcDecl->templateSpec, esContext);
		if (ev.progress != symbol_component::EvaluationProgress::Evaluating)
		{
			throw NotResolvableException();
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

		FinishEvaluatingPotentialGenericSymbol(pa, funcDecl, funcDecl->templateSpec, esContext);
	}

	TypeTsysList& EvaluateFuncSymbol(
		const ParsingArguments& invokerPa,
		ForwardFunctionDeclaration* funcDecl,
		ITsys* parentDeclType,
		EvaluateSymbolContext* esContext
	)
	{
		auto eval = ProcessArguments(invokerPa, funcDecl, funcDecl->templateSpec, parentDeclType, esContext);
		if (eval)
		{
			if (funcDecl->needResolveTypeFromStatement)
			{
				if (auto rootFuncDecl = dynamic_cast<FunctionDeclaration*>(funcDecl))
				{
					EnsureFunctionBodyParsed(rootFuncDecl);
					EvaluateStat(eval.declPa, rootFuncDecl->statement, true, esContext);

					if (eval.evaluatedTypes.Count() == 0)
					{
						eval.evaluatedTypes.Add(eval.declPa.tsys->Void());
						return FinishEvaluatingPotentialGenericSymbol(eval.declPa, funcDecl, funcDecl->templateSpec, esContext);
					}
					else
					{
						return eval.evaluatedTypes;
					}
				}
				else
				{
					throw NotResolvableException();
				}
			}
			else
			{
				TypeToTsysNoVta(eval.declPa, funcDecl->type, eval.evaluatedTypes, IsMemberFunction(funcDecl));
				return FinishEvaluatingPotentialGenericSymbol(eval.declPa, funcDecl, funcDecl->templateSpec, esContext);
			}
		}
		else
		{
			return eval.evaluatedTypes;
		}
	}

	/***********************************************************************
	EvaluateClassSymbol: Evaluate base types for a class
	***********************************************************************/

	TypeTsysList& EvaluateForwardClassSymbol(
		const ParsingArguments& invokerPa,
		ForwardClassDeclaration* classDecl,
		ITsys* parentDeclType,
		EvaluateSymbolContext* esContext
	)
	{
		if (esContext)
		{
			// not implemented
			throw 0;
		}

		auto eval = ProcessArguments(invokerPa, classDecl, classDecl->templateSpec, parentDeclType, esContext);
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
			return FinishEvaluatingPotentialGenericSymbol(eval.declPa, classDecl, classDecl->templateSpec, esContext);
		}
		else
		{
			return eval.evaluatedTypes;
		}
	}

	symbol_component::Evaluation& EvaluateClassSymbol(
		const ParsingArguments& invokerPa,
		ClassDeclaration* classDecl,
		ITsys* parentDeclType,
		EvaluateSymbolContext* esContext
	)
	{
		if (esContext)
		{
			// not implemented
			throw 0;
		}

		EvaluateForwardClassSymbol(invokerPa, classDecl, parentDeclType, esContext);
		auto eval = ProcessArguments(invokerPa, classDecl, classDecl->templateSpec, parentDeclType, esContext);
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

	/***********************************************************************
	EvaluateTypeAliasSymbol: Evaluate the declared type for an alias
	***********************************************************************/

	TypeTsysList& EvaluateTypeAliasSymbol(
		const ParsingArguments& invokerPa,
		TypeAliasDeclaration* usingDecl,
		ITsys* parentDeclType,
		EvaluateSymbolContext* esContext
	)
	{
		auto eval = ProcessArguments(invokerPa, usingDecl, usingDecl->templateSpec, parentDeclType, esContext);
		if (eval)
		{
			TypeToTsysNoVta(eval.declPa, usingDecl->type, eval.evaluatedTypes);
			return FinishEvaluatingPotentialGenericSymbol(eval.declPa, usingDecl, usingDecl->templateSpec, esContext);
		}
		else
		{
			return eval.evaluatedTypes;
		}
	}

	/***********************************************************************
	EvaluateValueAliasSymbol: Evaluate the declared value for an alias
	***********************************************************************/

	TypeTsysList& EvaluateValueAliasSymbol(
		const ParsingArguments& invokerPa,
		ValueAliasDeclaration* usingDecl,
		ITsys* parentDeclType,
		EvaluateSymbolContext* esContext
	)
	{
		auto eval = ProcessArguments(invokerPa, usingDecl, usingDecl->templateSpec, parentDeclType, esContext);
		if (eval)
		{
			if (usingDecl->needResolveTypeFromInitializer)
			{
				ExprTsysList tsys;
				ExprToTsysNoVta(eval.declPa, usingDecl->expr, tsys);
				for (vint i = 0; i < tsys.Count(); i++)
				{
					auto entityType = tsys[i].tsys;
					if (entityType->GetType() == TsysType::Zero)
					{
						entityType = eval.declPa.tsys->Int();
					}
					if (!eval.evaluatedTypes.Contains(entityType))
					{
						eval.evaluatedTypes.Add(entityType);
					}
				}
			}
			else
			{
				TypeToTsysNoVta(eval.declPa, usingDecl->type, eval.evaluatedTypes);
			}

			return FinishEvaluatingPotentialGenericSymbol(eval.declPa, usingDecl, usingDecl->templateSpec, esContext);
		}
		else
		{
			return eval.evaluatedTypes;
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
			throw NotResolvableException();
		}
	}
}