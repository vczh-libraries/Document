#include "Ast.h"
#include "Ast_Decl.h"
#include "Ast_Type.h"
#include "Ast_Expr.h"
#include "Ast_Stat.h"
#include "Parser.h"
#include "EvaluateSymbol.h"

/***********************************************************************
Resolving
***********************************************************************/

bool Resolving::ContainsSameSymbol(const Ptr<Resolving>& a, const Ptr<Resolving>& b)
{
	if (a && b)
	{
		return !From(a->items).Intersect(b->items).IsEmpty();
	}
	else
	{
		return !a && !b;
	}
}

bool Resolving::IsResolvedToType(const Ptr<Resolving>& resolving)
{
	if (resolving)
	{
		auto& items = resolving->items;
		for (vint i = 0; i < items.Count(); i++)
		{
			if (items[i].symbol->kind != symbol_component::SymbolKind::Namespace)
			{
				return true;
			}
		}
		return false;
	}
	else
	{
		// namespaces are always resolved
		return true;
	}
}

Symbol* Resolving::EnsureSingleSymbol(const Ptr<Resolving>& resolving)
{
	if (resolving && resolving->items.Count() == 1)
	{
		return resolving->items[0].symbol;
	}
	return nullptr;
}

Symbol* Resolving::EnsureSingleSymbol(const Ptr<Resolving>& resolving, symbol_component::SymbolKind kind)
{
	auto symbol = EnsureSingleSymbol(resolving);
	if (symbol && symbol->kind == kind)
	{
		return symbol;
	}
	return nullptr;
}

void Resolving::AddSymbol(const ParsingArguments& pa, Ptr<Resolving>& resolving, Symbol* symbol)
{
	if (!resolving)
	{
		resolving = MakePtr<Resolving>();
	}

	auto parent = symbol->GetParentScope();
	switch (parent->kind)
	{
	case CLASS_SYMBOL_KIND:
		{
			auto classDecl = parent->GetAnyForwardDecl<ForwardClassDeclaration>();
			auto& tsys = symbol_type_resolving::EvaluateForwardClassSymbol(pa, classDecl.Obj(), nullptr, nullptr);
			resolving->items.Add({ tsys[0],symbol });
		}
		break;
	default:
		resolving->items.Add({ nullptr,symbol });
	}
}

/***********************************************************************
AST
***********************************************************************/

#define CPPDOC_ACCEPT(NAME) void NAME::Accept(ITypeVisitor* visitor) { visitor->Visit(this); }
CPPDOC_TYPE_LIST(CPPDOC_ACCEPT)
#undef CPPDOC_ACCEPT

#define CPPDOC_ACCEPT(NAME) void NAME::Accept(IDeclarationVisitor* visitor) { visitor->Visit(this); }
CPPDOC_DECL_LIST(CPPDOC_ACCEPT)
#undef CPPDOC_ACCEPT

#define CPPDOC_ACCEPT(NAME) void NAME::Accept(IExprVisitor* visitor) { visitor->Visit(this); }
CPPDOC_EXPR_LIST(CPPDOC_ACCEPT)
#undef CPPDOC_ACCEPT

#define CPPDOC_ACCEPT(NAME) void NAME::Accept(IStatVisitor* visitor) { visitor->Visit(this); }
CPPDOC_STAT_LIST(CPPDOC_ACCEPT)
#undef CPPDOC_ACCEPT

/***********************************************************************
Type
***********************************************************************/

#define CORE_PROCESSOR										\
	[&name, &resolving](const Ptr<IdType>& _idType)			\
	{														\
		name = &_idType->name;								\
		resolving = &_idType->resolving;					\
	},														\
	[&name, &resolving](const Ptr<ChildType>& _childType)	\
	{														\
		name = &_childType->name;							\
		resolving = &_childType->resolving;					\
	}														\

void GetCategoryTypeResolving(const Ptr<Category_Id_Child_Type>& expr, CppName*& name, Ptr<Resolving>*& resolving)
{
	MatchCategoryType(
		expr,
		CORE_PROCESSOR
	);
}

void GetCategoryTypeResolving(const Ptr<Category_Id_Child_Generic_Root_Type>& expr, CppName*& name, Ptr<Resolving>*& resolving)
{
	MatchCategoryType(
		expr,
		CORE_PROCESSOR,
		[&name, &resolving](const Ptr<GenericType>& _genericType)
		{
			GetCategoryTypeResolving(_genericType->type, name, resolving);
		},
		[](const Ptr<RootType>)
		{
		}
	);
}

#undef CORE_PROCESSOR

/***********************************************************************
Expressions
***********************************************************************/

#define CORE_PROCESSOR									\
	[&idExpr](const Ptr<IdExpr>& _idExpr)				\
	{													\
		idExpr = _idExpr;								\
	},													\
	[&childExpr](const Ptr<ChildExpr>& _childExpr)		\
	{													\
		childExpr = _childExpr;							\
	}													\

void CastCategoryExpr(const Ptr<Category_Id_Child_Expr>& expr, Ptr<IdExpr>& idExpr, Ptr<ChildExpr>& childExpr)
{
	MatchCategoryExpr(
		expr,
		CORE_PROCESSOR
	);
}

void CastCategoryExpr(const Ptr<Category_Id_Child_Generic_Expr>& expr, Ptr<IdExpr>& idExpr, Ptr<ChildExpr>& childExpr, Ptr<GenericExpr>& genericExpr)
{
	MatchCategoryExpr(
		expr,
		CORE_PROCESSOR,
		[&idExpr, &childExpr, &genericExpr](const Ptr<GenericExpr>& _genericExpr)
		{
			genericExpr = _genericExpr;
			CastCategoryExpr(genericExpr->expr, idExpr, childExpr);
		}
	);
}

#undef CORE_PROCESSOR

#define CORE_PROCESSOR										\
	[&name, &resolving](const Ptr<IdExpr>& _idExpr)			\
	{														\
		name = &_idExpr->name;								\
		resolving = &_idExpr->resolving;					\
	},														\
	[&name, &resolving](const Ptr<ChildExpr>& _childExpr)	\
	{														\
		name = &_childExpr->name;							\
		resolving = &_childExpr->resolving;					\
	}														\

void GetCategoryExprResolving(const Ptr<Category_Id_Child_Expr>& expr, CppName*& name, Ptr<Resolving>*& resolving)
{
	MatchCategoryExpr(
		expr,
		CORE_PROCESSOR
	);
}

void GetCategoryExprResolving(const Ptr<Category_Id_Child_Generic_Expr>& expr, CppName*& name, Ptr<Resolving>*& resolving)
{
	MatchCategoryExpr(
		expr,
		CORE_PROCESSOR,
		[&name, &resolving](const Ptr<GenericExpr>& _genericExpr)
		{
			GetCategoryExprResolving(_genericExpr->expr, name, resolving);
		}
	);
}

#undef CORE_PROCESSOR