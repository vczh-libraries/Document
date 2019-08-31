#include "Ast_Stat.h"
#include "Ast_Expr.h"
#include "Ast_Resolving.h"

/***********************************************************************
EvaluateStatVisitor
***********************************************************************/

class EvaluateStatVisitor : public Object, public IStatVisitor
{
public:
	const ParsingArguments&			pa;
	bool&							resolvingFunctionType;
	EvaluateSymbolContext*			esContext;
	GenericArgContext*				gaContext;

	EvaluateStatVisitor(const ParsingArguments& _pa, bool& _resolvingFunctionType, EvaluateSymbolContext* _esContext)
		:pa(_pa)
		, resolvingFunctionType(_resolvingFunctionType)
		, esContext(_esContext)
	{
		if (esContext)
		{
			gaContext = &esContext->gaContext;
		}
	}

	void Evaluate(const ParsingArguments& spa, Ptr<Stat> stat)
	{
		EvaluateStatVisitor visitor(spa, resolvingFunctionType, esContext);
		stat->Accept(&visitor);
	}

	void Visit(EmptyStat* self) override
	{
	}

	void Visit(BlockStat* self) override
	{
		auto spa = pa.WithContext(self->symbol);
		for (vint i = 0; i < self->stats.Count(); i++)
		{
			Evaluate(spa, self->stats[i]);
		}
	}

	void Visit(DeclStat* self) override
	{
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			auto decl = self->decls[i];
			if (auto varDecl = decl.Cast<VariableDeclaration>())
			{
				EvaluateVariableDeclaration(pa, varDecl.Obj(), gaContext);
			}
			else
			{
				throw NotResolvableException();
			}
		}
	}

	void Visit(ExprStat* self) override
	{
		ExprTsysList types;
		ExprToTsysNoVta(pa, self->expr, types, gaContext);
	}

	void Visit(LabelStat* self) override
	{
	}

	void Visit(DefaultStat* self) override
	{
		self->stat->Accept(this);
	}

	void Visit(CaseStat* self) override
	{
		{
			ExprTsysList types;
			ExprToTsysNoVta(pa, self->expr, types, gaContext);
		}
		self->stat->Accept(this);
	}

	void Visit(GotoStat* self) override
	{
	}

	void Visit(BreakStat* self) override
	{
	}

	void Visit(ContinueStat* self) override
	{
	}

	void Visit(WhileStat* self) override
	{
		auto spa = pa.WithContext(self->symbol);
		if (self->varExpr)
		{
			EvaluateVariableDeclaration(spa, self->varExpr.Obj(), gaContext);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsysNoVta(spa, self->expr, types, gaContext);
		}
		Evaluate(spa, self->stat);
	}

	void Visit(DoWhileStat* self) override
	{
		{
			ExprTsysList types;
			ExprToTsysNoVta(pa, self->expr, types, gaContext);
		}
		Evaluate(pa, self->stat);
	}

	void Visit(ForEachStat* self) override
	{
		ExprTsysList types;
		ExprToTsysNoVta(pa, self->expr, types, gaContext);

		if (self->varDecl->needResolveTypeFromInitializer)
		{
			auto symbol = self->varDecl->symbol;
			auto& ev = symbol->GetEvaluationForUpdating_NFb();
			if (ev.progress == symbol_component::EvaluationProgress::NotEvaluated)
			{
				ev.progress = symbol_component::EvaluationProgress::Evaluating;
				ev.Allocate();

				for (vint i = 0; i < types.Count(); i++)
				{
					auto tsys = types[i].tsys;
					{
						TsysCV cv;
						TsysRefType refType;
						auto entity = tsys->GetEntity(cv, refType);
						if (entity->GetType() == TsysType::Array)
						{
							auto resolved = ResolvePendingType(pa, self->varDecl->type, { nullptr,ExprTsysType::LValue,entity->GetElement()->LRefOf() });
							if (!ev.Get().Contains(resolved))
							{
								ev.Get().Add(resolved);
							}
							types.RemoveAt(i--);
						}
					}
				}

				if (types.Count() > 0)
				{
					ExprTsysList virtualExprTypes;
					{
						// *begin(container)

						auto placeholderExpr = MakePtr<PlaceholderExpr>();
						placeholderExpr->types = &types;

						auto beginExpr = MakePtr<IdExpr>();
						beginExpr->name.name = L"begin";
						beginExpr->name.type = CppNameType::Normal;

						auto callExpr = MakePtr<FuncAccessExpr>();
						callExpr->expr = beginExpr;
						callExpr->arguments.Add({ placeholderExpr,false });

						auto derefExpr = MakePtr<PrefixUnaryExpr>();
						derefExpr->op = CppPrefixUnaryOp::Dereference;
						derefExpr->opName.name = L"*";
						derefExpr->opName.type = CppNameType::Operator;
						derefExpr->operand = callExpr;

						ExprToTsysNoVta(pa, derefExpr, virtualExprTypes, gaContext);
					}
					{
						// *container.begin()

						auto placeholderExpr = MakePtr<PlaceholderExpr>();
						placeholderExpr->types = &types;

						auto beginExpr = MakePtr<IdExpr>();
						beginExpr->name.name = L"begin";
						beginExpr->name.type = CppNameType::Normal;

						auto fieldExpr = MakePtr<FieldAccessExpr>();
						fieldExpr->expr = placeholderExpr;
						fieldExpr->name = beginExpr;
						fieldExpr->type = CppFieldAccessType::Dot;

						auto callExpr = MakePtr<FuncAccessExpr>();
						callExpr->expr = fieldExpr;

						auto derefExpr = MakePtr<PrefixUnaryExpr>();
						derefExpr->op = CppPrefixUnaryOp::Dereference;
						derefExpr->opName.name = L"*";
						derefExpr->opName.type = CppNameType::Operator;
						derefExpr->operand = callExpr;

						ExprToTsysNoVta(pa, derefExpr, virtualExprTypes, gaContext);
					}

					for (vint i = 0; i < virtualExprTypes.Count(); i++)
					{
						auto resolved = ResolvePendingType(pa, self->varDecl->type, virtualExprTypes[i]);
						if (!ev.Get().Contains(resolved))
						{
							ev.Get().Add(resolved);
						}
					}
				}

				if (ev.Get().Count() == 0)
				{
					throw NotResolvableException();
				}
				ev.progress = symbol_component::EvaluationProgress::Evaluated;
			}
		}
		else
		{
			EvaluateVariableDeclaration(pa, self->varDecl.Obj(), gaContext);
		}

		auto spa = pa.WithContext(self->symbol);
		Evaluate(spa, self->stat);
	}

	void Visit(ForStat* self) override
	{
		auto spa = pa.WithContext(self->symbol);
		for (vint i = 0; i < self->varDecls.Count(); i++)
		{
			EvaluateVariableDeclaration(spa, self->varDecls[i].Obj(), gaContext);
		}
		if (self->init)
		{
			ExprTsysList types;
			ExprToTsysNoVta(spa, self->init, types, gaContext);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsysNoVta(spa, self->expr, types, gaContext);
		}
		if (self->effect)
		{
			ExprTsysList types;
			ExprToTsysNoVta(spa, self->effect, types, gaContext);
		}
		Evaluate(spa, self->stat);
	}

	void Visit(IfElseStat* self) override
	{
		auto spa = pa.WithContext(self->symbol);
		for (vint i = 0; i < self->varDecls.Count(); i++)
		{
			EvaluateVariableDeclaration(spa, self->varDecls[i].Obj(), gaContext);
		}
		if (self->varExpr)
		{
			EvaluateVariableDeclaration(spa, self->varExpr.Obj(), gaContext);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsysNoVta(spa, self->expr, types, gaContext);
		}
		Evaluate(spa, self->trueStat);
		if (self->falseStat)
		{
			Evaluate(spa, self->falseStat);
		}
	}

	void Visit(SwitchStat* self) override
	{
		auto spa = pa.WithContext(self->symbol);
		if (self->varExpr)
		{
			EvaluateVariableDeclaration(spa, self->varExpr.Obj(), gaContext);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsysNoVta(spa, self->expr, types, gaContext);
		}
		Evaluate(spa, self->stat);
	}

	void Visit(TryCatchStat* self) override
	{
		self->tryStat->Accept(this);
		auto spa = pa.WithContext(self->symbol);
		if (self->exception)
		{
			EvaluateVariableDeclaration(spa, self->exception.Obj(), gaContext);
		}
		Evaluate(spa, self->catchStat);
	}

	void Visit(ReturnStat* self) override
	{
		ExprTsysList types;
		if (self->expr)
		{
			ExprToTsysNoVta(pa, self->expr, types, gaContext);
		}

		if (pa.functionBodySymbol && resolvingFunctionType)
		{
			resolvingFunctionType = false;
			TypeTsysList tsyses;
			if (self->expr)
			{
				for (vint i = 0; i < types.Count(); i++)
				{
					auto tsys = types[i].tsys;
					if (!tsyses.Contains(tsys))
					{
						tsyses.Add(tsys);
					}
				}
			}
			else
			{
				tsyses.Add(pa.tsys->Void());
			}
			symbol_type_resolving::SetFuncTypeByReturnStat(pa, pa.functionBodySymbol->GetImplDecl_NFb<FunctionDeclaration>().Obj(), tsyses, esContext);
		}
	}

	void Visit(__Try__ExceptStat* self) override
	{
		{
			ExprTsysList types;
			ExprToTsysNoVta(pa, self->expr, types, gaContext);
		}
		self->tryStat->Accept(this);
		self->exceptStat->Accept(this);
	}

	void Visit(__Try__FinallyStat* self) override
	{
		self->tryStat->Accept(this);
		self->finallyStat->Accept(this);
	}

	void Visit(__LeaveStat* self) override
	{
	}

	void Visit(__IfExistsStat* self) override
	{
		{
			ExprTsysList types;
			ExprToTsysNoVta(pa, self->expr, types, gaContext);
		}
		self->stat->Accept(this);
	}

	void Visit(__IfNotExistsStat* self) override
	{
		{
			ExprTsysList types;
			ExprToTsysNoVta(pa, self->expr, types, gaContext);
		}
		self->stat->Accept(this);
	}
};

/***********************************************************************
EvaluateDeclarationVisitor
***********************************************************************/

class EvaluateDeclarationVisitor : public Object, public IDeclarationVisitor
{
public:
	const ParsingArguments&			pa;

	EvaluateDeclarationVisitor(const ParsingArguments& _pa)
		:pa(_pa)
	{
	}

	void Visit(ForwardVariableDeclaration* self) override
	{
	}

	void Visit(ForwardFunctionDeclaration* self) override
	{
	}

	void Visit(ForwardEnumDeclaration* self) override
	{
	}

	void Visit(ForwardClassDeclaration* self) override
	{
	}

	void Visit(VariableDeclaration* self) override
	{
		EvaluateVariableDeclaration(pa, self, nullptr);
	}

	void Visit(FunctionDeclaration* self) override
	{
		EnsureFunctionBodyParsed(self);
		symbol_type_resolving::EvaluateFuncSymbol(pa, self);

		if(self->initList.Count() > 0)
		{
			auto classDecl = self->symbol->GetParentScope()->GetImplDecl_NFb<ClassDeclaration>();
			if (!classDecl)
			{
				throw NotResolvableException();
			}
			if (self->name.type != CppNameType::Constructor)
			{
				throw NotResolvableException();
			}

			auto newPa = pa.WithContext(self->symbol);
			for (vint i = 0; i < self->initList.Count(); i++)
			{
				auto& item = self->initList[i];
				{
					auto pVars = classDecl->symbol->TryGetChildren_NFb(item.f0->name.name);
					if (!pVars) goto SKIP_RESOLVING_FIELD;
					if (pVars->Count() != 1) goto SKIP_RESOLVING_FIELD;
					auto varSymbol = pVars->Get(0).Obj();
					if (varSymbol->kind != symbol_component::SymbolKind::Variable) goto SKIP_RESOLVING_FIELD;

					item.f0->resolving = MakePtr<Resolving>();
					item.f0->resolving->resolvedSymbols.Add(varSymbol);
				}
			SKIP_RESOLVING_FIELD:;
				ExprTsysList types;
				ExprToTsysNoVta(newPa, item.f1, types);
			}
		}

		if (!self->needResolveTypeFromStatement)
		{
			auto fpa = pa.WithContext(self->symbol);
			EvaluateStat(fpa, self->statement, false, nullptr);
		}
	}

	void Visit(EnumItemDeclaration* self) override
	{
		if (self->value)
		{
			ExprTsysList types;
			ExprToTsysNoVta(pa, self->value, types);
		}
	}

	void Visit(EnumDeclaration* self) override
	{
		for (vint i = 0; i < self->items.Count(); i++)
		{
			self->items[i]->Accept(this);
		}
	}

	void Visit(ClassDeclaration* self) override
	{
		symbol_type_resolving::EvaluateClassSymbol(pa, self);
		auto dpa = pa.WithContext(self->symbol);
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			EvaluateDeclaration(dpa, self->decls[i].f1);
		}
	}

	void Visit(NestedAnonymousClassDeclaration* self) override
	{
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			EvaluateDeclaration(pa, self->decls[i]);
		}
	}

	void Visit(UsingNamespaceDeclaration* self) override
	{
	}

	void Visit(UsingSymbolDeclaration* self) override
	{
	}

	void Visit(TypeAliasDeclaration* self) override
	{
		symbol_type_resolving::EvaluateTypeAliasSymbol(pa, self);
	}

	void Visit(ValueAliasDeclaration* self) override
	{
		symbol_type_resolving::EvaluateValueAliasSymbol(pa, self);
	}

	void Visit(NamespaceDeclaration* self) override
	{
		auto dpa = pa.WithContext(self->symbol);
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			EvaluateDeclaration(dpa, self->decls[i]);
		}
	}
};

/***********************************************************************
Evaluate
***********************************************************************/

void EvaluateStat(const ParsingArguments& pa, Ptr<Stat> s, bool resolvingFunctionType, EvaluateSymbolContext* esContext)
{
	EvaluateStatVisitor visitor(pa, resolvingFunctionType, esContext);
	s->Accept(&visitor);
}

void EvaluateVariableDeclaration(const ParsingArguments& pa, VariableDeclaration* decl, GenericArgContext* gaContext)
{
	symbol_type_resolving::EvaluateVarSymbol(pa, decl);
	if (!decl->needResolveTypeFromInitializer && decl->initializer)
	{
		for (vint i = 0; i < decl->initializer->arguments.Count(); i++)
		{
			ExprTsysList types;
			bool typesVta = false;
			ExprToTsysInternal(pa, decl->initializer->arguments[i].item, types, typesVta);
			if (typesVta != decl->initializer->arguments[i].isVariadic)
			{
				throw NotResolvableException();
			}
		}
	}
}

void EvaluateDeclaration(const ParsingArguments& pa, Ptr<Declaration> decl)
{
	auto dpa = pa.WithContext(decl->symbol);
	EvaluateDeclarationVisitor visitor(dpa);
	decl->Accept(&visitor);
}

void EvaluateProgram(const ParsingArguments& pa, Ptr<Program> program)
{
	for (vint i = 0; i < program->decls.Count(); i++)
	{
		EvaluateDeclaration(pa, program->decls[i]);
	}
}
