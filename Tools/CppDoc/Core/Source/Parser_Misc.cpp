#include "Parser.h"

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