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
		return !From(a->resolvedSymbols).Intersect(b->resolvedSymbols).IsEmpty();
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
		auto& symbols = resolving->resolvedSymbols;
		for (vint i = 0; i < symbols.Count(); i++)
		{
			if (symbols[i]->kind != symbol_component::SymbolKind::Namespace)
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
	if (resolving && resolving->resolvedSymbols.Count() == 1)
	{
		return resolving->resolvedSymbols[0];
	}
	return nullptr;
}

Symbol* Resolving::EnsureSingleSymbol(const Ptr<Resolving>& resolving, symbol_component::SymbolKind kind)
{
	if (auto symbol = EnsureSingleSymbol(resolving))
	{
		if (symbol->kind == kind)
		{
			return symbol;
		}
	}
	return nullptr;
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