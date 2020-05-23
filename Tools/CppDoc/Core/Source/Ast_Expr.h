#ifndef VCZH_DOCUMENT_CPPDOC_AST_EXPR
#define VCZH_DOCUMENT_CPPDOC_AST_EXPR

#include "Ast.h"

/***********************************************************************
Visitor
***********************************************************************/

#define CPPDOC_EXPR_LIST(F)\
	F(PlaceholderExpr)\
	F(LiteralExpr)\
	F(ThisExpr)\
	F(NullptrExpr)\
	F(ParenthesisExpr)\
	F(CastExpr)\
	F(TypeidExpr)\
	F(SizeofExpr)\
	F(ThrowExpr)\
	F(DeleteExpr)\
	F(IdExpr)\
	F(ChildExpr)\
	F(FieldAccessExpr)\
	F(ArrayAccessExpr)\
	F(FuncAccessExpr)\
	F(CtorAccessExpr)\
	F(NewExpr)\
	F(UniversalInitializerExpr)\
	F(PostfixUnaryExpr)\
	F(PrefixUnaryExpr)\
	F(BinaryExpr)\
	F(IfExpr)\
	F(GenericExpr)\
	F(BuiltinFuncAccessExpr)\

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

class Category_Id_Child_Generic_Expr : public Expr
{
public:
};

class Category_Id_Child_Expr : public Category_Id_Child_Generic_Expr
{
public:
	Ptr<Resolving>					resolving;
};

class PlaceholderExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	ExprTsysList*					types = nullptr;
};

/***********************************************************************
Expressions
***********************************************************************/

class LiteralExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	List<RegexToken>				tokens;
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

	Ptr<Expr>						expr;
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

	CppCastType						castType;
	Ptr<Type>						type;
	Ptr<Expr>						expr;
};

class TypeidExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	Ptr<Type>						type;
	Ptr<Expr>						expr;
};

class SizeofExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	bool							ellipsis = false;
	Ptr<Type>						type;
	Ptr<Expr>						expr;
};

class ThrowExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	Ptr<Expr>						expr;
};

class DeleteExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	bool							globalOperator = false;
	bool							arrayDelete = false;
	Ptr<Expr>						expr;
};

class IdExpr : public Category_Id_Child_Expr
{
public:
	IExprVisitor_ACCEPT;

	CppName							name;
};

class ChildExpr : public Category_Id_Child_Expr
{
public:
	IExprVisitor_ACCEPT;

	Ptr<Type>						classType;
	CppName							name;
};

enum class CppFieldAccessType
{
	Dot,			// .
	Arrow,			// ->
};

class FieldAccessExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	CppName									opName;
	Ptr<Resolving>							opResolving;

	CppFieldAccessType						type;
	Ptr<Expr>								expr;
	Ptr<Category_Id_Child_Generic_Expr>		name;
};

class ArrayAccessExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	CppName							opName;
	Ptr<Resolving>					opResolving;

	Ptr<Expr>						expr;
	Ptr<Expr>						index;
};

class FuncAccessExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	CppName							opName;
	Ptr<Resolving>					opResolving;

	Ptr<Expr>						expr;
	VariadicList<Ptr<Expr>>			arguments;
};

class CtorAccessExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	Ptr<Type>						type;
	Ptr<Initializer>				initializer;
};

class NewExpr : public CtorAccessExpr
{
public:
	IExprVisitor_ACCEPT;

	bool							globalOperator = false;
	VariadicList<Ptr<Expr>>			placementArguments;
};

class UniversalInitializerExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	VariadicList<Ptr<Expr>>			arguments;
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

	Ptr<Resolving>					opResolving;
	CppPostfixUnaryOp				op;
	CppName							opName;
	Ptr<Expr>						operand;
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

	Ptr<Resolving>					opResolving;
	CppPrefixUnaryOp				op;
	CppName							opName;
	Ptr<Expr>						operand;
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

	Ptr<Resolving>					opResolving;
	vint							precedence = -1;
	CppBinaryOp						op;
	CppName							opName;
	Ptr<Expr>						left;
	Ptr<Expr>						right;
};

class IfExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	Ptr<Expr>						condition;
	Ptr<Expr>						left;
	Ptr<Expr>						right;
};

class GenericExpr : public Category_Id_Child_Generic_Expr
{
public:
	IExprVisitor_ACCEPT;

	Ptr<Category_Id_Child_Expr>		expr;
	VariadicList<GenericArgument>	arguments;
};

class BuiltinFuncAccessExpr : public Expr
{
public:
	IExprVisitor_ACCEPT;

	Ptr<Type>						returnType;
	Ptr<IdExpr>						name;
	VariadicList<GenericArgument>	arguments;
};

/***********************************************************************
Expressions
***********************************************************************/

template<typename TId, typename TChild, typename TGeneric>
void MatchCategoryExpr(const Ptr<Category_Id_Child_Generic_Expr>& expr, TId&& processId, TChild&& processChild, TGeneric&& processGeneric)
{
	if (auto idExpr = expr.Cast<IdExpr>())
	{
		processId(idExpr);
	}
	else if (auto childExpr = expr.Cast<ChildExpr>())
	{
		processChild(childExpr);
	}
	else if (auto genericExpr = expr.Cast<GenericExpr>())
	{
		processGeneric(genericExpr);
	}
	else
	{
		throw TypeCheckerException();
	}
}

template<typename TId, typename TChild>
void MatchCategoryExpr(const Ptr<Category_Id_Child_Expr>& expr, TId&& processId, TChild&& processChild)
{
	if (auto idExpr = expr.Cast<IdExpr>())
	{
		processId(idExpr);
	}
	else if (auto childExpr = expr.Cast<ChildExpr>())
	{
		processChild(childExpr);
	}
	else
	{
		throw TypeCheckerException();
	}
}

#endif