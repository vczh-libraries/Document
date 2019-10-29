#include "Ast_Resolving.h"

namespace symbol_type_resolving
{
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
				auto ta = newPa.taContext;
				while (ta)
				{
					if (ta->symbolToApply == declSymbol)
					{
						// for all taContext that after this ta
						// those type arguments are useless because they replace internal symbols
						// so we use this ta's parent
						// in this case this symbol will be evaluated to a GenericFunction if it has TemplateSpec
						// this is for ensuring GetEvaluationKind doesn't return EvaluationKind::Instantiated
						newPa.taContext = ta->parent;
						break;
					}
					ta = ta->parent;
				}
			}
			return newPa;
		}
	}

	TypeTsysList& FinishEvaluatingPotentialGenericSymbol(const ParsingArguments& declPa, Declaration* decl, Ptr<TemplateSpec> spec, TemplateArgumentContext* argumentsToApply)
	{
		auto& ev = GetCorrectEvaluation(declPa, decl, spec, argumentsToApply);
		auto& evaluatedTypes = ev.Get();

		if (spec && !argumentsToApply)
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

	Eval ProcessArguments(const ParsingArguments& invokerPa, Declaration* decl, Ptr<TemplateSpec> spec, ITsys*& parentDeclType, TemplateArgumentContext* argumentsToApply)
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
		TemplateArgumentContext* argumentsToApply
	)
	{
		auto symbol = funcDecl->symbol;
		auto& ev = GetCorrectEvaluation(pa, funcDecl, funcDecl->templateSpec, argumentsToApply);
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
			if (funcDecl->needResolveTypeFromStatement)
			{
				if (auto rootFuncDecl = dynamic_cast<FunctionDeclaration*>(funcDecl))
				{
					EnsureFunctionBodyParsed(rootFuncDecl);
					EvaluateStat(eval.declPa, rootFuncDecl->statement, true, argumentsToApply);

					if (eval.evaluatedTypes.Count() == 0)
					{
						eval.evaluatedTypes.Add(eval.declPa.tsys->Void());
						return FinishEvaluatingPotentialGenericSymbol(eval.declPa, funcDecl, funcDecl->templateSpec, argumentsToApply);
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
				return FinishEvaluatingPotentialGenericSymbol(eval.declPa, funcDecl, funcDecl->templateSpec, argumentsToApply);
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
		TemplateArgumentContext* argumentsToApply
	)
	{
		auto eval = ProcessArguments(invokerPa, classDecl, classDecl->templateSpec, parentDeclType, argumentsToApply);
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
				if (!classDecl) throw NotResolvableException();

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
			throw NotResolvableException();
		}
	}

	/***********************************************************************
	EvaluateTypeAliasSymbol: Evaluate the declared type for an alias
	***********************************************************************/

	TypeTsysList& EvaluateTypeAliasSymbol(
		const ParsingArguments& invokerPa,
		TypeAliasDeclaration* usingDecl,
		ITsys* parentDeclType,
		TemplateArgumentContext* argumentsToApply
	)
	{
		auto eval = ProcessArguments(invokerPa, usingDecl, usingDecl->templateSpec, parentDeclType, argumentsToApply);
		if (eval)
		{
			TypeToTsysNoVta(eval.declPa, usingDecl->type, eval.evaluatedTypes);
			return FinishEvaluatingPotentialGenericSymbol(eval.declPa, usingDecl, usingDecl->templateSpec, argumentsToApply);
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
		TemplateArgumentContext* argumentsToApply
	)
	{
		auto eval = ProcessArguments(invokerPa, usingDecl, usingDecl->templateSpec, parentDeclType, argumentsToApply);
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

			return FinishEvaluatingPotentialGenericSymbol(eval.declPa, usingDecl, usingDecl->templateSpec, argumentsToApply);
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