#include "Parser.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"

Ptr<Type> ParsePrimitiveType(Ptr<CppTokenCursor>& cursor, CppPrimitivePrefix prefix)
{
#define TEST_SINGLE_KEYWORD(KEYWORD)\
	if (TestToken(cursor, L#KEYWORD)) return MakePtr<PrimitiveType>(prefix, CppPrimitiveType::_##KEYWORD)

#define TEST_LONG_KEYWORD(KEYWORD)\
	if (TestToken(cursor, L#KEYWORD)) return MakePtr<PrimitiveType>(prefix, CppPrimitiveType::_long_##KEYWORD)

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

bool SkipSpecifiers(Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::LBRACKET, CppTokens::LBRACKET))
	{
		int counter = 0;
		while (cursor)
		{
			if (counter == 0)
			{
				if (TestToken(cursor, CppTokens::RBRACKET, CppTokens::RBRACKET))
				{
					return true;
				}
			}

			if (TestToken(cursor, CppTokens::LBRACKET))
			{
				counter++;
			}
			else if (TestToken(cursor, CppTokens::RBRACKET))
			{
				counter--;
			}
		}
		throw StopParsingException(cursor);
	}
	else if (TestToken(cursor, L"__declspec"))
	{
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		int counter = 1;

		while (cursor)
		{
			if (TestToken(cursor, CppTokens::LPARENTHESIS))
			{
				counter++;
			}
			else if (TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				counter--;

				if (counter == 0)
				{
					return true;
				}
			}
		}
		throw StopParsingException(cursor);
	}
	return false;
}

bool ParseCppName(CppName& name, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::OPERATOR, false))
	{
		auto& token = cursor->token;
		name.operatorName = true;
		name.tokenCount = 1;
		name.name = L"operator ";
		name.nameTokens[0] = token;
		cursor = cursor->Next();

		auto nameCursor = cursor;

#define OPERATOR_NAME_1(TOKEN1)\
		if (TestToken(nameCursor, CppTokens::TOKEN1))\
		{\
			name.tokenCount += 1;\
			name.nameTokens[1] = cursor->token;\
			name.name += WString(name.nameTokens[1].reading, name.nameTokens[1].length);\
		}\
		else\

#define OPERATOR_NAME_2(TOKEN1, TOKEN2)\
		if (TestToken(nameCursor, CppTokens::TOKEN1, CppTokens::TOKEN2))\
		{\
			name.tokenCount += 2;\
			name.nameTokens[1] = cursor->token;\
			name.nameTokens[2] = cursor->Next()->token;\
			name.name += WString(name.nameTokens[1].reading, name.nameTokens[1].length + name.nameTokens[2].length);\
		}\
		else\

#define OPERATOR_NAME_3(TOKEN1, TOKEN2, TOKEN3)\
		if (TestToken(nameCursor, CppTokens::TOKEN1, CppTokens::TOKEN2, CppTokens::TOKEN3))\
		{\
			name.tokenCount += 3;\
			name.nameTokens[1] = cursor->token;\
			name.nameTokens[2] = cursor->Next()->token;\
			name.nameTokens[3] = cursor->Next()->Next()->token;\
			name.name += WString(name.nameTokens[1].reading, name.nameTokens[1].length + name.nameTokens[2].length + name.nameTokens[3].length);\
		}\
		else\

		OPERATOR_NAME_3(NEW, LPARENTHESIS, RPARENTHESIS)
		OPERATOR_NAME_1(NEW)
		OPERATOR_NAME_3(DELETE, LPARENTHESIS, RPARENTHESIS)
		OPERATOR_NAME_1(DELETE)

		OPERATOR_NAME_1(COMMA)
		OPERATOR_NAME_2(LPARENTHESIS, RPARENTHESIS)
		OPERATOR_NAME_2(LBRACKET, RBRACKET)
		OPERATOR_NAME_3(SUB, GT, MUL)
		OPERATOR_NAME_2(SUB, GT)

		OPERATOR_NAME_2(NOT, EQ)
		OPERATOR_NAME_1(NOT)
		OPERATOR_NAME_2(EQ, EQ)
		OPERATOR_NAME_1(EQ)
		OPERATOR_NAME_2(REVERT, EQ)
		OPERATOR_NAME_1(REVERT)
		OPERATOR_NAME_2(XOR, EQ)
		OPERATOR_NAME_1(XOR)

		OPERATOR_NAME_3(AND, AND, EQ)
		OPERATOR_NAME_2(AND, AND)
		OPERATOR_NAME_2(AND, EQ)
		OPERATOR_NAME_1(AND)
		OPERATOR_NAME_3(OR, OR, EQ)
		OPERATOR_NAME_2(OR, OR)
		OPERATOR_NAME_2(OR, EQ)
		OPERATOR_NAME_1(OR)

		OPERATOR_NAME_2(MUL, EQ)
		OPERATOR_NAME_1(MUL)
		OPERATOR_NAME_2(DIV, EQ)
		OPERATOR_NAME_1(DIV)
		OPERATOR_NAME_2(PERCENT, EQ)
		OPERATOR_NAME_1(PERCENT)

		OPERATOR_NAME_2(ADD, EQ)
		OPERATOR_NAME_2(ADD, ADD)
		OPERATOR_NAME_1(ADD)
		OPERATOR_NAME_2(SUB, EQ)
		OPERATOR_NAME_2(SUB, SUB)
		OPERATOR_NAME_1(SUB)

		OPERATOR_NAME_3(LT, LT, EQ)
		OPERATOR_NAME_2(LT, LT)
		OPERATOR_NAME_2(LT, EQ)
		OPERATOR_NAME_1(LT)
		OPERATOR_NAME_3(GT, GT, EQ)
		OPERATOR_NAME_2(GT, GT)
		OPERATOR_NAME_2(GT, EQ)
		OPERATOR_NAME_1(GT)
		{
			throw StopParsingException(cursor);
		}
		
		cursor = nameCursor;
		return true;

#undef OPERATOR_NAME_1
#undef OPERATOR_NAME_2
#undef OPERATOR_NAME_3
	}
	else if (TestToken(cursor, CppTokens::ID, false))
	{
		auto& token = cursor->token;
		name.operatorName = false;
		name.tokenCount = 1;
		name.name = WString(token.reading, token.length);
		name.nameTokens[0] = token;
		cursor = cursor->Next();
		return true;
	}
	return false;
}

Ptr<Declarator> ParseShortDeclarator(ParsingArguments& pa, Ptr<Type> typeResult, DeclaratorRestriction dr, Ptr<CppTokenCursor>& cursor)
{
	while (SkipSpecifiers(cursor));

	if (TestToken(cursor, L"alignas"))
	{
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		ParseExpr(pa, cursor);
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		return ParseShortDeclarator(pa, typeResult, dr, cursor);
	}
	else if (TestToken(cursor, CppTokens::MUL))
	{
		TestToken(cursor, L"__ptr32") || TestToken(cursor, L"__ptr64");
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::Ptr;
		type->type = typeResult;
		return ParseShortDeclarator(pa, type, dr, cursor);
	}
	else if (TestToken(cursor, CppTokens::AND, CppTokens::AND))
	{
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::RRef;
		type->type = typeResult;
		return ParseShortDeclarator(pa, type, dr, cursor);
	}
	else if (TestToken(cursor, CppTokens::AND))
	{
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::LRef;
		type->type = typeResult;
		return ParseShortDeclarator(pa, type, dr, cursor);
	}
	else if (TestToken(cursor, CppTokens::CONSTEXPR))
	{
		auto type = typeResult.Cast<DecorateType>();
		if (!type)
		{
			type = MakePtr<DecorateType>();
			type->type = typeResult;
		}
		type->isConstExpr = true;
		return ParseShortDeclarator(pa, type, dr, cursor);
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
		return ParseShortDeclarator(pa, type, dr, cursor);
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
		return ParseShortDeclarator(pa, type, dr, cursor);
	}

#define CALLING_CONVENTION_KEYWORD(KEYWORD, NAME)\
	else if (TestToken(cursor, L#KEYWORD))\
	{\
		auto type = typeResult.Cast<FunctionType>();\
		if (!type)\
		{\
			type = MakePtr<FunctionType>();\
			type->waitingForParameters = true;\
			type->returnType = typeResult;\
		}\
		type->callingConvention = CppCallingConvention::NAME;\
		return ParseShortDeclarator(pa, type, dr, cursor);\
	}\

	CALLING_CONVENTION_KEYWORD(__cdecl, CDecl)
	CALLING_CONVENTION_KEYWORD(__clrcall, ClrCall)
	CALLING_CONVENTION_KEYWORD(__stdcall, StdCall)
	CALLING_CONVENTION_KEYWORD(__fastcall, FastCall)
	CALLING_CONVENTION_KEYWORD(__thiscall, ThisCall)
	CALLING_CONVENTION_KEYWORD(__vectorcall, VectorCall)

#undef CALLING_CONVENTION_KEYWORD

	else
	{
		{
			auto oldCursor = cursor;
			try
			{
				auto type = ParseShortType(pa, cursor);
				cursor = oldCursor;
			}
			catch (const StopParsingException&)
			{
				goto CHECK_CPP_NAME;
			}

			cursor = oldCursor;
			throw StopParsingException(cursor);
		}
	CHECK_CPP_NAME:
		{
			auto declarator = MakePtr<Declarator>();
			declarator->type = typeResult;
			if (dr != DeclaratorRestriction::Zero)
			{
				if (!ParseCppName(declarator->name, cursor))
				{
					if (dr == DeclaratorRestriction::One)
					{
						throw StopParsingException(cursor);
					}
				}
			}
			return declarator;
		}
	}
}

Ptr<Declarator> ParseLongDeclarator(ParsingArguments& pa, Ptr<Type> typeResult, DeclaratorRestriction dr, Ptr<CppTokenCursor>& cursor)
{
	auto declarator = ParseShortDeclarator(pa, typeResult, dr, cursor);
	Ptr<IdenticalType> identicalType;
	{
		auto oldCursor = cursor;
		if (!declarator->name)
		{
			if (TestToken(cursor, CppTokens::LPARENTHESIS, CppTokens::RPARENTHESIS))
			{
				cursor = oldCursor;
				goto GIVE_UP;
			}

			if (TestToken(cursor, CppTokens::LPARENTHESIS))
			{
				identicalType = MakePtr<IdenticalType>();
				identicalType->type = declarator->type;

				try
				{
					declarator = ParseLongDeclarator(pa, identicalType, dr, cursor);
				}
				catch (const StopParsingException&)
				{
					cursor = oldCursor;
					declarator->type = identicalType->type;
					goto GIVE_UP;
				}

				RequireToken(cursor, CppTokens::RPARENTHESIS);
			}
		}
	}
GIVE_UP:

	if (TestToken(cursor, CppTokens::LBRACKET, false))
	{
		while (true)
		{
			if (TestToken(cursor, CppTokens::LBRACKET))
			{
				Ptr<Type>& typeToUpdate = identicalType ? identicalType->type : declarator->type;

				auto type = MakePtr<ArrayType>();
				type->type = typeToUpdate;
				typeToUpdate = type;

				if (!TestToken(cursor, CppTokens::RBRACKET))
				{
					type->expr = ParseExpr(pa, cursor);
					RequireToken(cursor, CppTokens::RBRACKET);
				}
			}
			else
			{
				break;
			}
		}
	}
	else if (TestToken(cursor, CppTokens::LPARENTHESIS))
	{
		Ptr<Type>& typeToUpdate = identicalType ? identicalType->type : declarator->type;

		auto type = typeToUpdate.Cast<FunctionType>();

		if (type)
		{
			if (type->waitingForParameters)
			{
				type->waitingForParameters = false;
				goto PARSE_PARAMETERS;
			}
		}

		type = MakePtr<FunctionType>();
		type->returnType = typeToUpdate;
		typeToUpdate = type;

	PARSE_PARAMETERS:
		while (!TestToken(cursor, CppTokens::RPARENTHESIS))
		{
			{
				List<Ptr<Declarator>> declarators;
				ParseDeclarator(pa, DeclaratorRestriction::Optional, InitializerRestriction::Optional, cursor, declarators);
				if (declarators.Count() != 1)
				{
					throw StopParsingException(cursor);
				}
				type->parameters.Add(declarators[0]);
			}

			if (TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				break;
			}
			else
			{
				RequireToken(cursor, CppTokens::COMMA);
			}
		}

		while (true)
		{
			if (TestToken(cursor, CppTokens::CONSTEXPR))
			{
				type->qualifierConstExpr = true;
			}
			else if (TestToken(cursor, CppTokens::CONST))
			{
				type->qualifierConst = true;
			}
			else if (TestToken(cursor, CppTokens::VOLATILE))
			{
				type->qualifierVolatile = true;
			}
			else if (TestToken(cursor, CppTokens::AND, CppTokens::AND))
			{
				type->qualifierRRef = true;
			}
			else if (TestToken(cursor, CppTokens::AND))
			{
				type->qualifierLRef = true;
			}
			else if (TestToken(cursor, CppTokens::OVERRIDE))
			{
				type->decoratorOverride = true;
			}
			else if (TestToken(cursor, CppTokens::SUB, CppTokens::GT))
			{
				if (auto primitiveType = type->returnType.Cast<PrimitiveType>())
				{
					if (primitiveType->primitive == CppPrimitiveType::_auto)
					{
						goto CONTINUE_RETURN_TYPE;
					}
				}
				throw StopParsingException(cursor);
			CONTINUE_RETURN_TYPE:

				List<Ptr<Declarator>> declarators;
				ParseDeclarator(pa, DeclaratorRestriction::Zero, InitializerRestriction::Zero, cursor, declarators);
				if (declarators.Count() != 1)
				{
					throw StopParsingException(cursor);
				}
				type->decoratorReturnType = declarators[0]->type;
			}
			else if (TestToken(cursor, CppTokens::NOEXCEPT))
			{
				type->decoratorNoExcept = true;
			}
			else if (TestToken(cursor, CppTokens::THROW))
			{
				type->decoratorThrow = true;

				RequireToken(cursor, CppTokens::LPARENTHESIS);
				while (!TestToken(cursor, CppTokens::RPARENTHESIS))
				{
					List<Ptr<Declarator>> declarators;
					ParseDeclarator(pa, DeclaratorRestriction::Zero, InitializerRestriction::Zero, cursor, declarators);
					if (declarators.Count() != 1)
					{
						throw StopParsingException(cursor);
					}
					type->exceptions.Add(declarators[0]->type);

					if (TestToken(cursor, CppTokens::RPARENTHESIS))
					{
						break;
					}
					else
					{
						RequireToken(cursor, CppTokens::COMMA);
					}
				}
			}
			else
			{
				break;
			}
		}
	}

	return declarator;
}

Ptr<Initializer> ParseInitializer(ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	auto initializer = MakePtr<Initializer>();

	if (TestToken(cursor, CppTokens::EQ))
	{
		initializer->initializerType = InitializerType::Equal;
	}
	else if (TestToken(cursor, CppTokens::LBRACE))
	{
		initializer->initializerType = InitializerType::Universal;
	}
	else if (TestToken(cursor, CppTokens::LPARENTHESIS))
	{
		initializer->initializerType = InitializerType::Constructor;
	}

	while (true)
	{
		initializer->arguments.Add(ParseExpr(pa, cursor));

		if (initializer->initializerType == InitializerType::Equal)
		{
			break;
		}
		else if (!TestToken(cursor, CppTokens::COMMA))
		{
			switch (initializer->initializerType)
			{
			case InitializerType::Universal:
				RequireToken(cursor, CppTokens::RBRACE);
				break;
			case InitializerType::Constructor:
				RequireToken(cursor, CppTokens::RPARENTHESIS);
				break;
			}
			break;
		}
	}

	return initializer;
}

void ParseDeclarator(ParsingArguments& pa, DeclaratorRestriction dr, InitializerRestriction ir, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators)
{
	Ptr<Type> typeResult = ParseLongType(pa, cursor);

	auto itemDr = dr == DeclaratorRestriction::Many ? DeclaratorRestriction::One : dr;
	while(true)
	{
		auto declarator = ParseLongDeclarator(pa, typeResult, itemDr, cursor);
		if (ir == InitializerRestriction::Optional)
		{
			if (TestToken(cursor, CppTokens::EQ, false) || TestToken(cursor, CppTokens::LBRACE, false) || TestToken(cursor, CppTokens::LPARENTHESIS, false))
			{
				declarator->initializer = ParseInitializer(pa, cursor);
			}
		}
		declarators.Add(declarator);

		if (dr != DeclaratorRestriction::Many)
		{
			break;
		}
		else if (!TestToken(cursor, CppTokens::COMMA))
		{
			break;
		}
	}
}