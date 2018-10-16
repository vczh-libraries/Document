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
	F(SizeofExpr)\
	F(ThrowExpr)\
	F(NewExpr)\
	F(DeleteExpr)\
	F(IdExpr)\
	F(ChildExpr)\
	F(FieldAccessExpr)\
	F(ArrayAccessExpr)\
	F(FuncAccessExpr)\
	F(PostfixUnaryExpr)\
	F(PrefixUnaryExpr)\
	F(BinaryExpr)\
	F(IfExpr)\

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

class SizeofExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	Ptr<Type>					type;
	Ptr<Expr>					expr;
};

class ThrowExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	Ptr<Expr>					expr;
};

class NewExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	bool						arrayNew = false;
	Ptr<Type>					type;
	List<Ptr<Expr>>				placementArguments;
	List<Ptr<Expr>>				arguments;
};

class DeleteExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	bool						arrayDelete = false;
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

enum class CppFieldAccessType
{
	Dot,			// .
	Arrow,			// ->
};

class FieldAccessExpr : public ResolvableExpr
{
public:
	IExprVisitor_ACCEPT;

	CppFieldAccessType		type;
	Ptr<Expr>				expr;
	CppName					name;
};

class ArrayAccessExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	Ptr<Expr>				expr;
	Ptr<Expr>				index;
};

class FuncAccessExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	Ptr<Type>				type;
	Ptr<Expr>				expr;
	List<Ptr<Expr>>			arguments;
};

enum class CppPostfixUnaryOp
{
	Increase,
	Decrease,
};

class PostfixUnaryExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	CppPostfixUnaryOp			op;
	CppName					opName;
	Ptr<Expr>				operand;
};

enum class CppPrefixUnaryOp
{
	Increase,
	Decrease,
	Revert,
	Not,
	Negative,
	Positive,
	AddressOf,
	Dereference,
};

class PrefixUnaryExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;
	
	CppPrefixUnaryOp		op;
	CppName					opName;
	Ptr<Expr>				operand;
};

enum class CppBinaryOp
{
	ValueFieldDeref, PtrFieldDeref,
	Mul, Div, Mod, Add, Sub, Shl, Shr,
	LT, GT, LE, GE, EQ, NE,
	BitAnd, BitOr, And, Or, Xor,
	Assign, MulAssign, DivAssign, ModAssign, AddAssign, SubAddisn, ShlAssign, ShrAssign, AndAssign, OrAssign, XorAssign,
	Comma,
};

class BinaryExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	vint					precedence = -1;
	CppBinaryOp				op;
	CppName					opName;
	Ptr<Expr>				left;
	Ptr<Expr>				right;
};

class IfExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	Ptr<Expr>				condition;
	Ptr<Expr>				left;
	Ptr<Expr>				right;
};

#endif