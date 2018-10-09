#ifndef VCZH_DOCUMENT_CPPDOC_AST_EXPR
#define VCZH_DOCUMENT_CPPDOC_AST_EXPR

#include "Ast.h"

/***********************************************************************
Visitor
***********************************************************************/

#define CPPDOC_EXPR_LIST(F)\
	F(LiteralExpr)\
	F(ThisExpr)\
	F(NullptrExpr)\
	F(ParenthesisExpr)\
	F(CastExpr)\
	F(TypeidExpr)\
	F(IdExpr)\
	F(ChildExpr)\

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
Preparation
***********************************************************************/

class ResolvableExpr : public Expr
{
public:
	Ptr<Resolving>			resolving;
};

/***********************************************************************
Expressions
***********************************************************************/

class LiteralExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	List<RegexToken>			tokens;
};

class ThisExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;
};

class NullptrExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;
};

class ParenthesisExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	Ptr<Expr>					expr;
};

enum class CppCastType
{
	CCast,
	DynamicCast,
	StaticCast,
	ConstCast,
	ReinterpretCast,
	SafeCast,
};

class CastExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	CppCastType					castType;
	Ptr<Type>					type;
	Ptr<Expr>					expr;
};

class TypeidExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	Ptr<Type>					type;
	Ptr<Expr>					expr;
};

class IdExpr : public ResolvableExpr
{
public:
	IExprVisitor_ACCEPT;

	CppName					name;
};

class ChildExpr : public ResolvableExpr
{
public:
	IExprVisitor_ACCEPT;

	Ptr<Type>				classType;
	CppName					name;
};

#endif