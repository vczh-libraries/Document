#include "Ast.h"
#include "Ast_Decl.h"
#include "Ast_Type.h"
#include "Ast_Expr.h"
#include "Ast_Stat.h"
#include "Parser.h"

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

ResolvedItem Resolving::EnsureSingleSymbol(const Ptr<Resolving>& resolving)
{
	if (resolving && resolving->items.Count() == 1)
	{
		return resolving->items[0];
	}
	return {};
}

ResolvedItem Resolving::EnsureSingleSymbol(const Ptr<Resolving>& resolving, symbol_component::SymbolKind kind)
{
	auto item = EnsureSingleSymbol(resolving);
	if (item.symbol && item.symbol->kind == kind)
	{
		return item;
	}
	return {};
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