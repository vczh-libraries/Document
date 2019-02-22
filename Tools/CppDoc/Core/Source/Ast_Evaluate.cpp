#include "Ast_Stat.h"
#include "Ast_Resolving.h"

/***********************************************************************
EvaluateStatVisitor
***********************************************************************/

class EvaluateStatVisitor : public Object, public IStatVisitor
{
public:
	ParsingArguments&					pa;

	EvaluateStatVisitor(ParsingArguments& _pa)
		:pa(_pa)
	{
	}

	void Visit(EmptyStat* self) override
	{
	}

	void Visit(BlockStat* self) override
	{
		ParsingArguments spa(pa, self->symbol);
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
		ExprToTsys(pa, self->expr, types);
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
			ExprToTsys(pa, self->expr, types);
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
		ParsingArguments spa(pa, self->symbol);
		if (self->varExpr)
		{
			EvaluateDeclaration(spa, self->varExpr);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(spa, self->expr, types);
		}
		EvaluateStat(spa, self->stat);
	}

	void Visit(DoWhileStat* self) override
	{
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types);
		}
		EvaluateStat(pa, self->stat);
	}

	void Visit(ForEachStat* self) override
	{
		ParsingArguments spa(pa, self->symbol);
		EvaluateDeclaration(spa, self->varDecl);
		{
			ExprTsysList types;
			ExprToTsys(spa, self->expr, types);
		}
		EvaluateStat(spa, self->stat);
	}

	void Visit(ForStat* self) override
	{
		ParsingArguments spa(pa, self->symbol);
		for (vint i = 0; i < self->varDecls.Count(); i++)
		{
			EvaluateDeclaration(spa, self->varDecls[i]);
		}
		if (self->init)
		{
			ExprTsysList types;
			ExprToTsys(spa, self->init, types);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(spa, self->expr, types);
		}
		if (self->effect)
		{
			ExprTsysList types;
			ExprToTsys(spa, self->effect, types);
		}
		EvaluateStat(spa, self->stat);
	}

	void Visit(IfElseStat* self) override
	{
		ParsingArguments spa(pa, self->symbol);
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
			ExprToTsys(spa, self->expr, types);
		}
		EvaluateStat(spa, self->trueStat);
		if (self->falseStat)
		{
			EvaluateStat(spa, self->falseStat);
		}
	}

	void Visit(SwitchStat* self) override
	{
		ParsingArguments spa(pa, self->symbol);
		if (self->varExpr)
		{
			EvaluateDeclaration(spa, self->varExpr);
		}
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(spa, self->expr, types);
		}
		EvaluateStat(spa, self->stat);
	}

	void Visit(TryCatchStat* self) override
	{
		self->tryStat->Accept(this);
		ParsingArguments spa(pa, self->symbol);
		if (self->exception)
		{
			EvaluateDeclaration(spa, self->exception);
		}
		EvaluateStat(spa, self->catchStat);
	}

	void Visit(ReturnStat* self) override
	{
		if (self->expr)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types);
		}
	}

	void Visit(__Try__ExceptStat* self) override
	{
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types);
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
			ExprToTsys(pa, self->expr, types);
		}
		self->stat->Accept(this);
	}

	void Visit(__IfNotExistsStat* self) override
	{
		{
			ExprTsysList types;
			ExprToTsys(pa, self->expr, types);
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
	ParsingArguments&					pa;

	EvaluateDeclarationVisitor(ParsingArguments& _pa)
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
		symbol_type_resolving::EvaluateSymbol(pa, self->symbol, self);
		if (!self->needResolveTypeFromInitializer && self->initializer && self->symbol->evaluation == SymbolEvaluation::NotEvaluated)
		{
			for (vint i = 0; i < self->initializer->arguments.Count(); i++)
			{
				ExprTsysList types;
				ExprToTsys(pa, self->initializer->arguments[i], types);
			}
		}
	}

	void Visit(FunctionDeclaration* self) override
	{
		EnsureFunctionBodyParsed(self);
		symbol_type_resolving::EvaluateSymbol(pa, self->symbol, self);
		if (!self->needResolveTypeFromStatement && self->symbol->evaluation == SymbolEvaluation::NotEvaluated)
		{
			ParsingArguments fpa(pa, self->symbol);
			fpa.funcSymbol = self->symbol;
			EvaluateStat(fpa, self->statement);
		}
	}

	void Visit(EnumItemDeclaration* self) override
	{
		if (self->value)
		{
			ExprTsysList types;
			ExprToTsys(pa, self->value, types);
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
		symbol_type_resolving::EvaluateSymbol(pa, self->symbol, self);
		ParsingArguments dpa(pa, self->symbol);
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			EvaluateDeclaration(dpa, self->decls[i].f1);
		}
	}

	void Visit(TypeAliasDeclaration* self) override
	{
	}

	void Visit(UsingNamespaceDeclaration* self) override
	{
	}

	void Visit(UsingDeclaration* self) override
	{
	}

	void Visit(NamespaceDeclaration* self) override
	{
		ParsingArguments dpa(pa, self->symbol);
		for (vint i = 0; i < self->decls.Count(); i++)
		{
			EvaluateDeclaration(dpa, self->decls[i]);
		}
	}
};

/***********************************************************************
Evaluate
***********************************************************************/

void EvaluateStat(ParsingArguments& pa, Ptr<Stat> s)
{
	EvaluateStatVisitor visitor(pa);
	s->Accept(&visitor);
}

void EvaluateDeclaration(ParsingArguments& pa, Ptr<Declaration> s)
{
	ParsingArguments dpa(pa, s->symbol);
	EvaluateDeclarationVisitor visitor(dpa);
	s->Accept(&visitor);
}

void EvaluateProgram(ParsingArguments& pa, Ptr<Program> program)
{
	for (vint i = 0; i < program->decls.Count(); i++)
	{
		EvaluateDeclaration(pa, program->decls[i]);
	}
}
