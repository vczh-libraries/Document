#include "Parser.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"

Ptr<Type> ParsePrimitiveType(Ptr<CppTokenCursor>& cursor, CppPrimitivePrefix prefix)
{
#define TEST_SINGLE_KEYWORD(KEYWORD)\
	if (TestToken(cursor, L ## #KEYWORD)) return MakePtr<PrimitiveType>(prefix, CppPrimitiveType::_##KEYWORD)

#define TEST_LONG_KEYWORD(KEYWORD)\
	if (TestToken(cursor, L ## #KEYWORD)) return MakePtr<PrimitiveType>(prefix, CppPrimitiveType::_long_##KEYWORD)

	TEST_SINGLE_KEYWORD(auto);
	TEST_SINGLE_KEYWORD(void);
	TEST_SINGLE_KEYWORD(bool);
	TEST_SINGLE_KEYWORD(char);
	TEST_SINGLE_KEYWORD(wchar_t);
	TEST_SINGLE_KEYWORD(char16_t);
	TEST_SINGLE_KEYWORD(char32_t);
	TEST_SINGLE_KEYWORD(short);
	TEST_SINGLE_KEYWORD(int);
	TEST_SINGLE_KEYWORD(__int8);
	TEST_SINGLE_KEYWORD(__int16);
	TEST_SINGLE_KEYWORD(__int32);
	TEST_SINGLE_KEYWORD(__int64);
	TEST_SINGLE_KEYWORD(float);
	TEST_SINGLE_KEYWORD(double);

	if (TestToken(cursor, L"long"))
	{
		TEST_LONG_KEYWORD(long);
		TEST_LONG_KEYWORD(double);
		return MakePtr<PrimitiveType>(prefix, CppPrimitiveType::_long);
	}

	if (prefix != CppPrimitivePrefix::_none)
	{
		throw StopParsingException(cursor);
	}
	else
	{
		return nullptr;
	}
#undef TEST_SINGLE_KEYWORD
#undef TEST_LONG_KEYWORD
}

Ptr<Type> ParseShortType(ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, L"signed"))
	{
		return ParsePrimitiveType(cursor, CppPrimitivePrefix::_signed);
	}
	else if (TestToken(cursor, L"unsigned"))
	{
		return ParsePrimitiveType(cursor, CppPrimitivePrefix::_unsigned);
	}
	else
	{
		{
			auto result = ParsePrimitiveType(cursor, CppPrimitivePrefix::_none);
			if (result) return result;
		}

		if (TestToken(cursor, L"decltype"))
		{
			RequireToken(cursor, CppTokens::LPARENTHESIS);
			auto type = MakePtr<DeclType>();
			type->expr = ParseExpr(pa, cursor);
			RequireToken(cursor, CppTokens::RPARENTHESIS);
			return type;
		}

		if (TestToken(cursor, L"constexpr"))
		{
			auto type = MakePtr<DecorateType>();
			type->isConstExpr = true;
			type->type = ParseShortType(pa, cursor);
			return type;
		}

		if (TestToken(cursor, L"const"))
		{
			auto type = MakePtr<DecorateType>();
			type->isConst = true;
			type->type = ParseShortType(pa, cursor);
			return type;
		}

		if (TestToken(cursor, L"volatile"))
		{
			auto type = MakePtr<DecorateType>();
			type->isVolatile = true;
			type->type = ParseShortType(pa, cursor);
			return type;
		}
	}

	throw StopParsingException(cursor);
}

Ptr<Type> ParseLongType(ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	Ptr<Type> typeResult = ParseShortType(pa, cursor);

	while (true)
	{
		if (TestToken(cursor, L"constexpr"))
		{
			auto type = MakePtr<DecorateType>();
			type->isConstExpr = true;
			type->type = typeResult;
			typeResult = type;
		}
		else if (TestToken(cursor, L"const"))
		{
			auto type = MakePtr<DecorateType>();
			type->isConst = true;
			type->type = typeResult;
			typeResult = type;
		}
		else if (TestToken(cursor, L"volatile"))
		{
			auto type = MakePtr<DecorateType>();
			type->isVolatile = true;
			type->type = typeResult;
			typeResult = type;
		}
		else if (TestToken(cursor, CppTokens::LT))
		{
			auto type = MakePtr<GenericType>();
			type->parent = typeResult;
			while (!TestToken(cursor, CppTokens::GT))
			{
				type->arguments.Add(ParseDeclarator(pa, DecoratorRestriction::Zero, InitializerRestriction::Zero, cursor)->type);
				if (TestToken(cursor, CppTokens::GT))
				{
					break;
				}
				else
				{
					RequireToken(cursor, CppTokens::COMMA);
				}
			}
			typeResult = type;
		}
		else if (TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT))
		{
			auto type = MakePtr<VariadicTemplateArgumentType>();
			type->type = typeResult;
			typeResult = type;
		}
		else
		{
			break;
		}
	}

	return typeResult;
}

Ptr<Declarator> ParseDeclarator(ParsingArguments& pa, DecoratorRestriction dr, InitializerRestriction ir, Ptr<CppTokenCursor>& cursor)
{
	Ptr<Type> typeResult = ParseLongType(pa, cursor);

	auto declarator = MakePtr<Declarator>();
	declarator->type = typeResult;
	return declarator;
}