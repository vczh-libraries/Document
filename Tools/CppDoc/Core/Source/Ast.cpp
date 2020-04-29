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