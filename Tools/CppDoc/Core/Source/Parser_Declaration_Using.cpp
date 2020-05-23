#include "Parser.h"
#include "Parser_Declaration.h"
#include "Parser_Declarator.h"
#include "Ast_Expr.h"

void ParseDeclaration_UsingNamespace(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	auto decl = MakePtr<UsingNamespaceDeclaration>();
	decl->ns = ParseType(pa, cursor);
	output.Add(decl);
	RequireToken(cursor, CppTokens::SEMICOLON);

	if (auto catIdChildType = decl->ns.Cast<Category_Id_Child_Type>())
	{
		if (!catIdChildType->resolving) throw StopParsingException(cursor);
		if (catIdChildType->resolving->items.Count() != 1) throw StopParsingException(cursor);
		auto symbol = catIdChildType->resolving->items[0].symbol;

		switch (symbol->kind)
		{
		case symbol_component::SymbolKind::Namespace:
			{
				if (pa.scopeSymbol && !(pa.scopeSymbol->usingNss.Contains(symbol)))
				{
					pa.scopeSymbol->usingNss.Add(symbol);
				}
			}
			break;
		default:
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

	auto decl = MakePtr<TypeAliasDeclaration>();
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
	auto decl = MakePtr<UsingSymbolDeclaration>();
	ParseTypeOrExpr(pa, pea_Full(), cursor, decl->type, decl->expr);
	RequireToken(cursor, CppTokens::SEMICOLON);
	output.Add(decl);

	Ptr<Resolving> resolving;
	Ptr<Type> classType;
	if (decl->expr)
	{
		if (auto childExpr = decl->expr.Cast<ChildExpr>())
		{
			resolving = childExpr->resolving;
			classType = childExpr->classType;
		}
	}
	else if (decl->type)
	{
		if (auto childType = decl->type.Cast<ChildType>())
		{
			resolving = childType->resolving;
			classType = childType->classType;
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
				pa.scopeSymbol->AddChild_NFb(cs.childSymbol->name, classType, cs.childSymbol);
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