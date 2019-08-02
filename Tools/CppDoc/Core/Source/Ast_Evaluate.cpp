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

	EvaluateStatVisitor(const ParsingArguments& _pa)
		:pa(_pa)
	{
	}

	void Visit(EmptyStat* self) override
	{
	}

	void Visit(BlockStat* self) override
	{
		auto spa = pa.WithContext(self->symbol);
		for (vint i = 0; i < self->stats.Count(); i++)
		{
			EvaluateStat(spa, self->stats[i]);
		}
	}

	void Visit(DeclStat* self) override
	{
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			EvaluateDeclaration(pa, self->decls[i]);
		}
	}

	void Visit(ExprStat* self) override
	{
		ExprTsysList types;
		ExprToTsysNoVta(pa, self->expr, types);
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
			ExprToTsysNoVta(pa, self->expr, types);
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
			EvaluateDeclaration(spa, self->varExpr);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsysNoVta(spa, self->expr, types);
		}
		EvaluateStat(spa, self->stat);
	}

	void Visit(DoWhileStat* self) override
	{
		{
			ExprTsysList types;
			ExprToTsysNoVta(pa, self->expr, types);
		}
		EvaluateStat(pa, self->stat);
	}

	void Visit(ForEachStat* self) override
	{
		ExprTsysList types;
		ExprToTsysNoVta(pa, self->expr, types);

		if (self->varDecl->needResolveTypeFromInitializer)
		{
			auto symbol = self->varDecl->symbol;
			auto& ev = symbol->evaluation;
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

						ExprToTsysNoVta(pa, derefExpr, virtualExprTypes);
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

						ExprToTsysNoVta(pa, derefExpr, virtualExprTypes);
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
			EvaluateDeclaration(pa, self->varDecl);
		}

		auto spa = pa.WithContext(self->symbol);
		EvaluateStat(spa, self->stat);
	}

	void Visit(ForStat* self) override
	{
		auto spa = pa.WithContext(self->symbol);
		for (vint i = 0; i < self->varDecls.Count(); i++)
		{
			EvaluateDeclaration(spa, self->varDecls[i]);
		}
		if (self->init)
		{
			ExprTsysList types;
			ExprToTsysNoVta(spa, self->init, types);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsysNoVta(spa, self->expr, types);
		}
		if (self->effect)
		{
			ExprTsysList types;
			ExprToTsysNoVta(spa, self->effect, types);
		}
		EvaluateStat(spa, self->stat);
	}

	void Visit(IfElseStat* self) override
	{
		auto spa = pa.WithContext(self->symbol);
		for (vint i = 0; i < self->varDecls.Count(); i++)
		{
			EvaluateDeclaration(spa, self->varDecls[i]);
		}
		if (self->varExpr)
		{
			EvaluateDeclaration(spa, self->varExpr);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsysNoVta(spa, self->expr, types);
		}
		EvaluateStat(spa, self->trueStat);
		if (self->falseStat)
		{
			EvaluateStat(spa, self->falseStat);
		}
	}

	void Visit(SwitchStat* self) override
	{
		auto spa = pa.WithContext(self->symbol);
		if (self->varExpr)
		{
			EvaluateDeclaration(spa, self->varExpr);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsysNoVta(spa, self->expr, types);
		}
		EvaluateStat(spa, self->stat);
	}

	void Visit(TryCatchStat* self) override
	{
		self->tryStat->Accept(this);
		auto spa = pa.WithContext(self->symbol);
		if (self->exception)
		{
			EvaluateDeclaration(spa, self->exception);
		}
		EvaluateStat(spa, self->catchStat);
	}

	void Visit(ReturnStat* self) override
	{
		ExprTsysList types;
		if (self->expr)
		{
			ExprToTsysNoVta(pa, self->expr, types);
		}

		if (pa.funcSymbol)
		{
			auto& ev = pa.funcSymbol->evaluation;
			if (ev.progress == symbol_component::EvaluationProgress::Evaluating)
			{
				if (ev.Count() == 0)
				{
					ev.Allocate();
					if (self->expr)
					{
						for (vint i = 0; i < types.Count(); i++)
						{
							auto tsys = types[i].tsys;
							if (!ev.Get().Contains(tsys))
							{
								ev.Get().Add(tsys);
							}
						}
					}
					else
					{
						ev.Get().Add(pa.tsys->Void());
					}
					symbol_type_resolving::FinishEvaluatingSymbol(pa, pa.funcSymbol->definition.Cast<FunctionDeclaration>().Obj());
				}
			}
		}
	}

	void Visit(__Try__ExceptStat* self) override
	{
		{
			ExprTsysList types;
			ExprToTsysNoVta(pa, self->expr, types);
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
			ExprToTsysNoVta(pa, self->expr, types);
		}
		self->stat->Accept(this);
	}

	void Visit(__IfNotExistsStat* self) override
	{
		{
			ExprTsysList types;
			ExprToTsysNoVta(pa, self->expr, types);
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
		symbol_type_resolving::EvaluateSymbol(pa, self);
		if (!self->needResolveTypeFromInitializer && self->initializer)
		{
			for (vint i = 0; i < self->initializer->arguments.Count(); i++)
			{
				ExprTsysList types;
				bool typesVta = false;
				ExprToTsysInternal(pa, self->initializer->arguments[i].item, types, typesVta);
				if (typesVta != self->initializer->arguments[i].isVariadic)
				{
					throw NotResolvableException();
				}
			}
		}
	}

	void Visit(FunctionDeclaration* self) override
	{
		EnsureFunctionBodyParsed(self);
		symbol_type_resolving::EvaluateSymbol(pa, self);

		if(self->initList.Count() > 0)
		{
			auto classDecl = self->symbol->parent->definition.Cast<ClassDeclaration>();
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
					vint index = classDecl->symbol->children.Keys().IndexOf(item.f0->name.name);
					if (index == -1) goto SKIP_RESOLVING_FIELD;
					auto& vars = classDecl->symbol->children.GetByIndex(index);
					if (vars.Count() != 1) goto SKIP_RESOLVING_FIELD;
					auto varSymbol = vars[0].Obj();
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
			EvaluateStat(fpa, self->statement);
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
		symbol_type_resolving::EvaluateSymbol(pa, self);
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
		symbol_type_resolving::EvaluateSymbol(pa, self);
	}

	void Visit(ValueAliasDeclaration* self) override
	{
		symbol_type_resolving::EvaluateSymbol(pa, self);
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

void EvaluateStat(const ParsingArguments& pa, Ptr<Stat> s)
{
	EvaluateStatVisitor visitor(pa);
	s->Accept(&visitor);
}

void EvaluateDeclaration(const ParsingArguments& pa, Ptr<Declaration> s)
{
	auto dpa = pa.WithContext(s->symbol);
	EvaluateDeclarationVisitor visitor(dpa);
	s->Accept(&visitor);
}

void EvaluateProgram(const ParsingArguments& pa, Ptr<Program> program)
{
	for (vint i = 0; i < program->decls.Count(); i++)
	{
		EvaluateDeclaration(pa, program->decls[i]);
	}
}
