#include "Parser.h"
#include "Parser_Declaration.h"

void ParseDeclaration_Namespace(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	RequireToken(cursor, CppTokens::DECL_NAMESPACE);
	if (TestToken(cursor, CppTokens::LBRACE))
	{
		// namespace { DECLARATION ...}
		// ignore it and add everything to its parent
		while (!TestToken(cursor, CppTokens::RBRACE))
		{
			ParseDeclaration(pa, cursor, output);
		}
	}
	else
	{
		// namespace NAME { :: NAME ...} { DECLARATION ...}
		auto contextSymbol = pa.scopeSymbol;
		Ptr<NamespaceDeclaration> topDecl;
		Ptr<NamespaceDeclaration> contextDecl;

		while (cursor)
		{
			// create AST
			auto decl = MakePtr<NamespaceDeclaration>();
			if (!topDecl)
			{
				topDecl = decl;
			}
			if (contextDecl)
			{
				contextDecl->decls.Add(decl);
			}
			contextDecl = decl;

			while (SkipSpecifiers(cursor));
			if (ParseCppName(decl->name, cursor))
			{
				// ensure all other overloadings are namespaces, and merge the scope with them
				contextSymbol = contextSymbol->AddForwardDeclToSymbol_NFb(decl, symbol_component::SymbolKind::Namespace);
				if (!contextSymbol)
				{
					throw StopParsingException(cursor);
				}
			}
			else
			{
				throw StopParsingException(cursor);
			}

			if (TestToken(cursor, CppTokens::LBRACE))
			{
				break;
			}
			else
			{
				RequireToken(cursor, CppTokens::COLON, CppTokens::COLON);
			}
		}

		auto newPa = pa.WithScope(contextSymbol);
		while (!TestToken(cursor, CppTokens::RBRACE))
		{
			ParseDeclaration(newPa, cursor, contextDecl->decls);
		}

		output.Add(topDecl);
	}
}