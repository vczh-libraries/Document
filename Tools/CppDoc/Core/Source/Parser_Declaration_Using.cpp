#include "Parser.h"
#include "Parser_Declaration.h"
#include "Parser_Declarator.h"
#include "Ast_Expr.h"

void ParseDeclaration_UsingNamespace(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	auto decl = Ptr(new UsingNamespaceDeclaration);
	decl->ns = ParseType(pa, cursor);
	output.Add(decl);
	RequireToken(cursor, CppTokens::SEMICOLON);

	if (auto catIdChildType = decl->ns.Cast<Category_Id_Child_Type>())
	{
		if (!catIdChildType->resolving) throw StopParsingException(cursor);

		bool added = false;
		for (vint i = 0; i < catIdChildType->resolving->items.Count(); i++)
		{
			auto symbol = catIdChildType->resolving->items[0].symbol;
			if (symbol->kind == symbol_component::SymbolKind::Namespace)
			{
				added = true;
				if (pa.scopeSymbol && !(pa.scopeSymbol->usingNss.Contains(symbol)))
				{
					pa.scopeSymbol->usingNss.Add(symbol);
				}
			}
		}

		if (!added)
		{
			throw StopParsingException(cursor);
		}
	}
	else
	{
		throw StopParsingException(cursor);
	}
}

bool ParseDeclaration_UsingAlias(const ParsingArguments& pa, Ptr<Symbol> specSymbol, Ptr<TemplateSpec> spec, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	auto oldCursor = cursor;
	ValidateForRootTemplateSpec(spec, cursor, false, false);

	CppName cppName;
	if (!ParseCppName(cppName, cursor) || !TestToken(cursor, CppTokens::EQ))
	{
		cursor = oldCursor;
		return false;
	}

	auto newPa = specSymbol ? pa.WithScope(specSymbol.Obj()) : pa;

	auto decl = Ptr(new TypeAliasDeclaration);
	decl->templateSpec = spec;
	decl->name = cppName;
	decl->type = ParseType(newPa, cursor);
	output.Add(decl);

	RequireToken(cursor, CppTokens::SEMICOLON);

	auto createdSymbol = pa.scopeSymbol->AddImplDeclToSymbol_NFb(decl, symbol_component::SymbolKind::TypeAlias, specSymbol);
	if (!createdSymbol)
	{
		throw StopParsingException(cursor);
	}

	if (decl->templateSpec)
	{
		decl->templateSpec->AssignDeclSymbol(createdSymbol);
	}
	return true;
}

void ParseDeclaration_UsingMember(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	auto decl = Ptr(new UsingSymbolDeclaration);
	ParseTypeOrExpr(pa, pea_Full(), cursor, decl->type, decl->expr);
	RequireToken(cursor, CppTokens::SEMICOLON);
	output.Add(decl);

	Ptr<Resolving> resolving;
	Ptr<ChildExpr> childExpr;
	Ptr<ChildType> childType;
	if (decl->expr)
	{
		if (childExpr = decl->expr.Cast<ChildExpr>())
		{
			resolving = childExpr->resolving;
		}
	}
	else if (decl->type)
	{
		if (childType = decl->type.Cast<ChildType>())
		{
			resolving = childType->resolving;
		}
	}

	if (!resolving) throw StopParsingException(cursor);
	for (vint i = 0; i < resolving->items.Count(); i++)
	{
		auto rawSymbolPtr = resolving->items[i].symbol;
		auto pSiblings = rawSymbolPtr->GetParentScope()->TryGetChildren_NFb(rawSymbolPtr->name);

		for (vint j = 0; j < pSiblings->Count(); j++)
		{
			auto& cs = pSiblings->Get(j);
			if (cs.childSymbol == rawSymbolPtr)
			{
				if (childExpr)
				{
					pa.scopeSymbol->AddChild_NFb(cs.childSymbol->name, { childExpr,cs.childSymbol });
				}
				else
				{
					pa.scopeSymbol->AddChild_NFb(cs.childSymbol->name, { childType,cs.childSymbol });
				}
			}
		}
	}
}

void ParseDeclaration_Using(const ParsingArguments& pa, Ptr<Symbol> specSymbol, Ptr<TemplateSpec> spec, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	RequireToken(cursor, CppTokens::DECL_USING);
	if (TestToken(cursor, CppTokens::DECL_NAMESPACE))
	{
		if (spec)
		{
			throw StopParsingException(cursor);
		}

		// using namespace TYPE;
		ParseDeclaration_UsingNamespace(pa, cursor, output);
	}
	else
	{
		// using NAME = TYPE;
		if (ParseDeclaration_UsingAlias(pa, specSymbol, spec, cursor, output))
		{
			return;
		}

		if (spec)
		{
			throw StopParsingException(cursor);
		}

		// using TYPE[::NAME];
		ParseDeclaration_UsingMember(pa, cursor, output);
	}
}