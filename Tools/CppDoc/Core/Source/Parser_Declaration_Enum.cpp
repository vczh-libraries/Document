#include "Parser.h"
#include "Parser_Declaration.h"

Ptr<EnumDeclaration> ParseDeclaration_Enum_NotConsumeSemicolon(const ParsingArguments& pa, bool forTypeDef, Ptr<CppTokenCursor>& cursor, List<Ptr<Declaration>>& output)
{
	RequireToken(cursor, CppTokens::DECL_ENUM);
	// enum [CLASS] NAME [: TYPE] ...
	bool enumClass = TestToken(cursor, CppTokens::DECL_CLASS);
	while (SkipSpecifiers(cursor));

	CppName cppName;
	bool isAnonymous = false;
	if (!ParseCppName(cppName, cursor))
	{
		if (enumClass)
		{
			throw StopParsingException(cursor);
		}
		else
		{
			isAnonymous = true;
			cppName.name = L"<anonymous>" + itow(pa.tsys->AllocateAnonymousCounter());
			cppName.type = CppNameType::Normal;
		}
	}

	Ptr<Type> baseType;
	if (TestToken(cursor, CppTokens::COLON))
	{
		baseType = ParseType(pa, cursor);
	}

	if (TestToken(cursor, CppTokens::SEMICOLON, false))
	{
		if (forTypeDef || isAnonymous)
		{
			throw StopParsingException(cursor);
		}

		// ... ;
		auto decl = MakePtr<ForwardEnumDeclaration>();
		decl->enumClass = enumClass;
		decl->name = cppName;
		decl->baseType = baseType;
		output.Add(decl);

		if (!pa.scopeSymbol->AddForwardDeclToSymbol_NFb(decl, symbol_component::SymbolKind::Enum))
		{
			throw StopParsingException(cursor);
		}
		return nullptr;
	}
	else
	{
		// ... { { IDENTIFIER [= EXPRESSION] ,... } };
		auto decl = MakePtr<EnumDeclaration>();
		decl->enumClass = enumClass;
		decl->name = cppName;
		decl->baseType = baseType;

		auto contextSymbol = pa.scopeSymbol->AddImplDeclToSymbol_NFb(decl, symbol_component::SymbolKind::Enum);
		if (!contextSymbol)
		{
			throw StopParsingException(cursor);
		}
		auto newPa = pa.WithScope(contextSymbol);

		RequireToken(cursor, CppTokens::LBRACE);
		while (!TestToken(cursor, CppTokens::RBRACE))
		{
			while (SkipSpecifiers(cursor));
			auto enumItem = MakePtr<EnumItemDeclaration>();
			if (!ParseCppName(enumItem->name, cursor)) throw StopParsingException(cursor);
			decl->items.Add(enumItem);
			if (!contextSymbol->AddImplDeclToSymbol_NFb(enumItem, symbol_component::SymbolKind::EnumItem))
			{
				throw StopParsingException(cursor);
			}

			if (!enumClass)
			{
				if (auto pEnumItems = pa.scopeSymbol->TryGetChildren_NFb(enumItem->name.name))
				{
					throw StopParsingException(cursor);
				}
				pa.scopeSymbol->AddChild_NFb(enumItem->name.name, contextSymbol->TryGetChildren_NFb(enumItem->name.name)->Get(0));
			}

			if (TestToken(cursor, CppTokens::EQ))
			{
				enumItem->value = ParseExpr(newPa, pea_Argument(), cursor);
			}

			if (!TestToken(cursor, CppTokens::COMMA))
			{
				RequireToken(cursor, CppTokens::RBRACE);
				break;
			}
		}

		output.Add(decl);
		return decl;
	}
}