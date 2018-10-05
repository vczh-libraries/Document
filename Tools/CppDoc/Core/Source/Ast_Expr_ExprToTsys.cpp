#include "Ast.h"
#include "Ast_Expr.h"
#include "Parser.h"

/***********************************************************************
ExprToTsys
***********************************************************************/

class ExprToTsysVisitor : public Object, public virtual IExprVisitor
{
public:
	List<ITsys*>&			result;
	ParsingArguments&		pa;

	ExprToTsysVisitor(ParsingArguments& _pa, List<ITsys*>& _result)
		:pa(_pa)
		, result(_result)
	{
	}

	void Visit(LiteralExpr* self)override
	{
		throw 0;
	}

	void Visit(ThisExpr* self)override
	{
		throw 0;
	}

	void Visit(NullptrExpr* self)override
	{
		throw 0;
	}

	void Visit(ParenthesisExpr* self)override
	{
		throw 0;
	}

	void Visit(CastExpr* self)override
	{
		throw 0;
	}

	void Visit(TypeidExpr* self)override
	{
		throw 0;
	}
};

// Resolve expressions to types
void ExprToTsys(ParsingArguments& pa, Ptr<Expr> e, List<ITsys*>& tsys)
{
	if (!e) throw IllegalExprException();
	ExprToTsysVisitor visitor(pa, tsys);
	e->Accept(&visitor);
}