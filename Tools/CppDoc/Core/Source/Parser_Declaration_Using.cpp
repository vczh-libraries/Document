#include "Parser.h"
#include "Parser_Declaration.h"

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
		auto decl = MakePtr<UsingNamespaceDeclaration>();
		decl->ns = ParseType(pa, cursor);
		output.Add(decl);
		RequireToken(cursor, CppTokens::SEMICOLON);

		if (auto catIdChildType = decl->ns.Cast<Category_Id_Child_Type>())
		{
			if (!catIdChildType->resolving) throw StopParsingException(cursor);
			if (catIdChildType->resolving->resolvedSymbols.Count() != 1) throw StopParsingException(cursor);
			auto symbol = catIdChildType->resolving->resolvedSymbols[0];

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
	else
	{
		{
			auto oldCursor = cursor;
			// using NAME = TYPE;
			ValidateForRootTemplateSpec(spec, cursor, false, false);

			CppName cppName;
			if (!ParseCppName(cppName, cursor) || !TestToken(cursor, CppTokens::EQ))
			{
				cursor = oldCursor;
				goto SKIP_TYPE_ALIAS;
			}

			auto newPa = specSymbol ? pa.WithScope(specSymbol.Obj()) : pa;

			auto decl = MakePtr<TypeAliasDeclaration>();
			decl->templateSpec = spec;
			decl->name = cppName;
			decl->type = ParseType(newPa, cursor);
			RequireToken(cursor, CppTokens::SEMICOLON);

			output.Add(decl);
			if (!pa.scopeSymbol->AddImplDeclToSymbol_NFb(decl, symbol_component::SymbolKind::TypeAlias, specSymbol))
			{
				throw StopParsingException(cursor);
			}

			return;
		}
	SKIP_TYPE_ALIAS:
		{
			if (spec)
			{
				throw StopParsingException(cursor);
			}

			// using TYPE[::NAME];
			auto decl = MakePtr<UsingSymbolDeclaration>();
			ParseTypeOrExpr(pa, pea_Full(), cursor, decl->type, decl->expr);
			RequireToken(cursor, CppTokens::SEMICOLON);
			output.Add(decl);

			Ptr<Resolving> resolving;
			if (decl->expr)
			{
				if (auto catIdChild = decl->expr.Cast<Category_Id_Child_Expr>())
				{
					resolving = catIdChild->resolving;
				}
			}
			else if (decl->type)
			{
				if (auto catIdChildType = decl->type.Cast<Category_Id_Child_Type>())
				{
					resolving = catIdChildType->resolving;
				}
			}

			if (!resolving) throw StopParsingException(cursor);
			for (vint i = 0; i < resolving->resolvedSymbols.Count(); i++)
			{
				auto rawSymbolPtr = resolving->resolvedSymbols[i];
				auto pSiblings = rawSymbolPtr->GetParentScope()->TryGetChildren_NFb(rawSymbolPtr->name);
				auto symbol = pSiblings->Get(pSiblings->IndexOf(rawSymbolPtr));

				switch (symbol->kind)
				{
				case symbol_component::SymbolKind::Enum:
				case symbol_component::SymbolKind::Class:
				case symbol_component::SymbolKind::Struct:
				case symbol_component::SymbolKind::Union:
				case symbol_component::SymbolKind::TypeAlias:
				case symbol_component::SymbolKind::EnumItem:
				case symbol_component::SymbolKind::Variable:
				case symbol_component::SymbolKind::ValueAlias:
					{
						if (auto pChildren = pa.scopeSymbol->TryGetChildren_NFb(symbol->name))
						{
							if (!pChildren->Contains(symbol.Obj()))
							{
								throw StopParsingException(cursor);
							}
						}
						else
						{
							pa.scopeSymbol->AddChild_NFb(symbol->name, symbol);
						}
					}
					break;
				case symbol_component::SymbolKind::FunctionSymbol:
					{
						if (auto pChildren = pa.scopeSymbol->TryGetChildren_NFb(symbol->name))
						{
							if (pChildren->Contains(symbol.Obj()))
							{
								goto SKIP_USING;
							}
							if (pChildren->Get(0)->kind != symbol_component::SymbolKind::FunctionSymbol)
							{
								throw StopParsingException(cursor);
							}
						}
						pa.scopeSymbol->AddChild_NFb(symbol->name, symbol);
					SKIP_USING:;
					}
					break;
				default:
					throw StopParsingException(cursor);
				}
			}
		}
	}
}