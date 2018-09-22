#ifndef VCZH_DOCUMENT_CPPDOC_AST_EXPR
#define VCZH_DOCUMENT_CPPDOC_AST_EXPR

#include "Ast.h"

/***********************************************************************
Visitor
***********************************************************************/

#define CPPDOC_EXPR_LIST(F)\
	F(LiteralExpr)\

#define CPPDOC_FORWARD(NAME) class NAME;
CPPDOC_EXPR_LIST(CPPDOC_FORWARD)
#undef CPPDOC_FORWARD

class IExprVisitor abstract : public virtual Interface
{
public:
#define CPPDOC_VISIT(NAME) virtual void Visit(NAME* self) = 0;
	CPPDOC_EXPR_LIST(CPPDOC_VISIT)
#undef CPPDOC_VISIT
};

#define IExprVisitor_ACCEPT void Accept(IExprVisitor* visitor)override

/***********************************************************************
Expressions
***********************************************************************/

class LiteralExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	List<RegexToken>			tokens;
};

#endif