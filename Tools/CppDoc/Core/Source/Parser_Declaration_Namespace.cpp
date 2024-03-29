#include "Parser.h"
#include "Parser_Declaration.h"

WString GenerateAnonymousNamespaceName(const ParsingArguments& pa)
{
	vint i = 0;
	while (true)
	{
		WString name = L"$__anonymous_namespace_" + itow(i);
		if (pa.scopeSymbol->TryGetChildren_NFb(name))
		{
			i++;
		}
		else
		{
			return name;
		}
	}
}

void ParseDeclaration_ExternC(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	// extern "C"
	RequireToken(cursor, CppTokens::DECL_EXTERN);
	RequireToken(cursor, CppTokens::STRING);

	if (TestToken(cursor, CppTokens::LBRACE))
	{
		while (!TestToken(cursor, CppTokens::RBRACE))
		{
			ParseDeclaration(pa, cursor, output);
		}
	}
	else
	{
		ParseDeclaration(pa, cursor, output);
	}
}

void ParseDeclaration_Namespace(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	RequireToken(cursor, CppTokens::DECL_NAMESPACE);
	if (TestToken(cursor, CppTokens::LBRACE))
	{
		// namespace { DECLARATION ...}
		// create an anonymous namespace
		auto decl = Ptr(new NamespaceDeclaration);
		decl->name.name = GenerateAnonymousNamespaceName(pa);
		decl->name.type = CppNameType::Normal;

		auto contextSymbol = pa.scopeSymbol->AddForwardDeclToSymbol_NFb(decl, symbol_component::SymbolKind::Namespace);
		if (!contextSymbol)
		{
			throw StopParsingException(cursor);
		}

		auto newPa = pa.WithScope(contextSymbol);
		while (!TestToken(cursor, CppTokens::RBRACE))
		{
			ParseDeclaration(newPa, cursor, decl->decls);
		}

		output.Add(decl);
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
			auto decl = Ptr(new NamespaceDeclaration);
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