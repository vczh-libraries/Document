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
		throw 0;
	}

	void Visit(BlockStat* self) override
	{
		throw 0;
	}

	void Visit(DeclStat* self) override
	{
		throw 0;
	}

	void Visit(ExprStat* self) override
	{
		throw 0;
	}

	void Visit(LabelStat* self) override
	{
		throw 0;
	}

	void Visit(DefaultStat* self) override
	{
		throw 0;
	}

	void Visit(CaseStat* self) override
	{
		throw 0;
	}

	void Visit(GotoStat* self) override
	{
		throw 0;
	}

	void Visit(BreakStat* self) override
	{
		throw 0;
	}

	void Visit(ContinueStat* self) override
	{
		throw 0;
	}

	void Visit(WhileStat* self) override
	{
		throw 0;
	}

	void Visit(DoWhileStat* self) override
	{
		throw 0;
	}

	void Visit(ForEachStat* self) override
	{
		throw 0;
	}

	void Visit(ForStat* self) override
	{
		throw 0;
	}

	void Visit(IfElseStat* self) override
	{
		throw 0;
	}

	void Visit(SwitchStat* self) override
	{
		throw 0;
	}

	void Visit(TryCatchStat* self) override
	{
		throw 0;
	}

	void Visit(ReturnStat* self) override
	{
		throw 0;
	}

	void Visit(__Try__ExceptStat* self) override
	{
		throw 0;
	}

	void Visit(__Try__FinallyStat* self) override
	{
		throw 0;
	}

	void Visit(__LeaveStat* self) override
	{
		throw 0;
	}

	void Visit(__IfExistsStat* self) override
	{
		throw 0;
	}

	void Visit(__IfNotExistsStat* self) override
	{
		throw 0;
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
