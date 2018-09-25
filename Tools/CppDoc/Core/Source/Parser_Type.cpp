#include "Parser.h"
#include "Ast_Type.h"

Ptr<Type> ParsePrimitiveType(Ptr<CppTokenCursor>& cursor, CppPrimitivePrefix prefix)
{
#define TEST_SINGLE_KEYWORD(TOKEN, KEYWORD)\
	if (TestToken(cursor, CppTokens::TOKEN)) return MakePtr<PrimitiveType>(prefix, CppPrimitiveType::_##KEYWORD)

#define TEST_LONG_KEYWORD(TOKEN, KEYWORD)\
	if (TestToken(cursor, CppTokens::TOKEN)) return MakePtr<PrimitiveType>(prefix, CppPrimitiveType::_long_##KEYWORD)

	TEST_SINGLE_KEYWORD(TYPE_AUTO, auto);
	TEST_SINGLE_KEYWORD(TYPE_VOID, void);
	TEST_SINGLE_KEYWORD(TYPE_BOOL, bool);
	TEST_SINGLE_KEYWORD(TYPE_CHAR, char);
	TEST_SINGLE_KEYWORD(TYPE_WCHAR_T, wchar_t);
	TEST_SINGLE_KEYWORD(TYPE_CHAR16_T, char16_t);
	TEST_SINGLE_KEYWORD(TYPE_CHAR32_T, char32_t);
	TEST_SINGLE_KEYWORD(TYPE_SHORT, short);
	TEST_SINGLE_KEYWORD(TYPE_INT, int);
	TEST_SINGLE_KEYWORD(TYPE___INT8, __int8);
	TEST_SINGLE_KEYWORD(TYPE___INT16, __int16);
	TEST_SINGLE_KEYWORD(TYPE___INT32, __int32);
	TEST_SINGLE_KEYWORD(TYPE___INT64, __int64);
	TEST_SINGLE_KEYWORD(TYPE_FLOAT, float);
	TEST_SINGLE_KEYWORD(TYPE_DOUBLE, double);

	if (TestToken(cursor, CppTokens::TYPE_LONG))
	{
		TEST_LONG_KEYWORD(TYPE_LONG, long);
		TEST_LONG_KEYWORD(TYPE_DOUBLE, double);
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
	if (TestToken(cursor, CppTokens::SIGNED))
	{
		return ParsePrimitiveType(cursor, CppPrimitivePrefix::_signed);
	}
	else if (TestToken(cursor, CppTokens::UNSIGNED))
	{
		return ParsePrimitiveType(cursor, CppPrimitivePrefix::_unsigned);
	}
	else if (TestToken(cursor, CppTokens::COLON, CppTokens::COLON, false))
	{
		return MakePtr<RootType>();
	}
	else
	{
		{
			auto result = ParsePrimitiveType(cursor, CppPrimitivePrefix::_none);
			if (result) return result;
		}

		if (TestToken(cursor, CppTokens::DECLTYPE))
		{
			RequireToken(cursor, CppTokens::LPARENTHESIS);
			auto type = MakePtr<DeclType>();
			type->expr = ParseExpr(pa, cursor);
			RequireToken(cursor, CppTokens::RPARENTHESIS);
			return type;
		}

		if (TestToken(cursor, CppTokens::CONSTEXPR))
		{
			auto type = MakePtr<DecorateType>();
			type->isConstExpr = true;
			type->type = ParseShortType(pa, cursor);
			return type;
		}

		if (TestToken(cursor, CppTokens::CONST))
		{
			auto type = MakePtr<DecorateType>();
			type->isConst = true;
			type->type = ParseShortType(pa, cursor);
			return type;
		}

		if (TestToken(cursor, CppTokens::VOLATILE))
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
		if (TestToken(cursor, CppTokens::CONSTEXPR))
		{
			auto type = typeResult.Cast<DecorateType>();
			if (!type)
			{
				type = MakePtr<DecorateType>();
				type->type = typeResult;
			}
			type->isConstExpr = true;
			typeResult = type;
		}
		else if (TestToken(cursor, CppTokens::CONST))
		{
			auto type = typeResult.Cast<DecorateType>();
			if (!type)
			{
				type = MakePtr<DecorateType>();
				type->type = typeResult;
			}
			type->isConst = true;
			typeResult = type;
		}
		else if (TestToken(cursor, CppTokens::VOLATILE))
		{
			auto type = typeResult.Cast<DecorateType>();
			if (!type)
			{
				type = MakePtr<DecorateType>();
				type->type = typeResult;
			}
			type->isVolatile = true;
			typeResult = type;
		}
		else if (TestToken(cursor, CppTokens::LT))
		{
			auto type = MakePtr<GenericType>();
			type->type = typeResult;
			while (!TestToken(cursor, CppTokens::GT))
			{
				{
					GenericArgument argument;
					List<Ptr<Declarator>> declarators;
					ParseDeclarator(pa, DeclaratorRestriction::Zero, InitializerRestriction::Zero, cursor, declarators);
					argument.type = declarators[0]->type;
					type->arguments.Add(argument);
				}

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