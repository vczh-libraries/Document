#include "Parser.h"
#include "Ast_Type.h"

/***********************************************************************
SkipSpecifiers
***********************************************************************/

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
			else
			{
				SkipToken(cursor);
			}
		}
		throw StopParsingException(cursor);
	}
	else if (TestToken(cursor, CppTokens::__DECLSPEC) || TestToken(cursor, CppTokens::__PRAGMA))
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
			else
			{
				SkipToken(cursor);
			}
		}
		throw StopParsingException(cursor);
	}
	return false;
}

/***********************************************************************
ParseCppName
***********************************************************************/

// operator PREDEFINED-OPERATOR
// ~IDENTIFIER			: destructor
// IDENTIFIER			: constructor / identifier
// operator new			:
// operator new[]		:
// operator delete		:
// operator delete[]	:
// operator "" TOKEN	: user-defined literals, but here it is ignored and become a normal name
bool ParseCppName(CppName& name, Ptr<CppTokenCursor>& cursor, bool forceSpecialMethod)
{
	{
		auto oldCursor = cursor;
		if (TestToken(cursor, CppTokens::OPERATOR) && TestToken(cursor, CppTokens::STRING))
		{
			auto idToken = cursor;
			SkipToken(cursor);

			name.type = CppNameType::Normal;
			name.tokenCount = 3;
			name.name = L"operator \"\" " + WString(idToken->token.reading, idToken->token.length);
			name.nameTokens[0] = oldCursor->token;
			name.nameTokens[1] = oldCursor->Next()->token;
			name.nameTokens[2] = oldCursor->Next()->Next()->token;
			return true;
		}
		else
		{
			cursor = oldCursor;
		}
	}

	if (TestToken(cursor, CppTokens::OPERATOR, false))
	{
		auto& token = cursor->token;
		name.type = CppNameType::Operator;
		name.tokenCount = 1;
		name.name = L"operator ";
		name.nameTokens[0] = token;
		SkipToken(cursor);

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

		OPERATOR_NAME_3(NEW, LBRACKET, RBRACKET)
		OPERATOR_NAME_1(NEW)
		OPERATOR_NAME_3(DELETE, LBRACKET, RBRACKET)
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

		OPERATOR_NAME_2(AND, AND)
		OPERATOR_NAME_2(AND, EQ)
		OPERATOR_NAME_1(AND)
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
			return false;
		}

		cursor = nameCursor;
		return true;

#undef OPERATOR_NAME_1
#undef OPERATOR_NAME_2
#undef OPERATOR_NAME_3
	}

	if (TestToken(cursor, CppTokens::REVERT, CppTokens::ID, false))
	{
		if (!forceSpecialMethod)
		{
			return false;
		}
		name.type = CppNameType::Destructor;
		name.tokenCount = 2;
		name.nameTokens[0] = cursor->token;
		name.nameTokens[1] = cursor->Next()->token;
		name.name = WString(cursor->token.reading, cursor->token.length + cursor->Next()->token.length);
		SkipToken(cursor);
		SkipToken(cursor);
		return true;
	}

	if (TestToken(cursor, CppTokens::ID, false))
	{
		name.type = CppNameType::Normal;
		name.tokenCount = 1;
		name.nameTokens[0] = cursor->token;
		name.name = WString(cursor->token.reading, cursor->token.length);
		SkipToken(cursor);
		return true;
	}

	return false;
}

/***********************************************************************
GetTypeWithoutMemberAndCC
***********************************************************************/

// Get TYPE in
//   TYPE __stdcall
//   TYPE CLASS::
//   TYPE __stdcall CLASS::
Ptr<Type> GetTypeWithoutMemberAndCC(Ptr<Type> type)
{
	if (auto memberType = type.Cast<MemberType>())
	{
		type = memberType->type;
	}
	if (auto ccType = type.Cast<CallingConventionType>())
	{
		type = ccType->type;
	}
	return type;
}

/***********************************************************************
ReplaceTypeInMemberAndCC
***********************************************************************/

// Replace TYPE with a new type and return the old one in
//   TYPE __stdcall
//   TYPE CLASS::
//   TYPE __stdcall CLASS::
Ptr<Type> ReplaceTypeInMemberAndCC(Ptr<Type>& type, Ptr<Type> typeToReplace)
{
	auto target = &type;
	if (auto memberType = target->Cast<MemberType>())
	{
		target = &memberType->type;
	}
	if (auto ccType = target->Cast<CallingConventionType>())
	{
		target = &ccType->type;
	}

	auto originalType = *target;
	*target = typeToReplace;
	return originalType;
}

/***********************************************************************
GetTypeWithoutMemberAndCC
***********************************************************************/

// Change
//   (TYPE __stdcall)         (PARAMETERS...) to TYPE (__stdcall)         (PARAMETERS...)
//   (TYPE CLASS::)           (PARAMETERS...) to TYPE (CLASS::)           (PARAMETERS...)
//   (TYPE __stdcall CLASS::) (PARAMETERS...) to TYPE (__stdcall CLASS::) (PARAMETERS...)
Ptr<Type> AdjustReturnTypeWithMemberAndCC(Ptr<FunctionType> functionType)
{
	Ptr<Type> adjustedType = functionType;
	Ptr<Type>* insertTarget = &adjustedType;

	if (auto memberType = functionType->returnType.Cast<MemberType>())
	{
		*insertTarget = memberType;
		functionType->returnType = memberType->type;
		insertTarget = &memberType->type;
		*insertTarget = functionType;
	}
	if (auto ccType = functionType->returnType.Cast<CallingConventionType>())
	{
		*insertTarget = ccType;
		functionType->returnType = ccType->type;
		insertTarget = &ccType->type;
		*insertTarget = functionType;
	}

	return adjustedType;
}

/***********************************************************************
RemoveArrayType
***********************************************************************/

Ptr<Type> RemoveArrayType(Ptr<Type> type, Ptr<Expr>& dim)
{
	if (auto dt = type.Cast<DecorateType>())
	{
		auto result = RemoveArrayType(dt->type, dim);
		if (!result) return nullptr;

		auto newType = MakePtr<DecorateType>();
		newType->isConst = dt->isConst;
		newType->isVolatile = dt->isVolatile;
		newType->type = result;

		return newType;
	}
	else if (auto at = type.Cast<ArrayType>())
	{
		dim = at->expr;
		return at->type;
	}
	else
	{
		return nullptr;
	}
}

/***********************************************************************
RemoveCVType
***********************************************************************/

Ptr<Type> RemoveCVType(Ptr<Type> type, bool& isConst, bool& isVolatile)
{
	if (auto dt = type.Cast<DecorateType>())
	{
		isConst |= dt->isConst;
		isVolatile |= dt->isVolatile;
		return RemoveCVType(dt->type, isConst, isVolatile);
	}
	else if (auto at = type.Cast<ArrayType>())
	{
		auto removed = RemoveCVType(at->type, isConst, isVolatile);
		if (removed == at->type)
		{
			return type;
		}
		else
		{
			auto newType = MakePtr<ArrayType>();
			newType->type = removed;
			newType->expr = at->expr;
			return newType;
		}
	}
	else
	{
		return type;
	}
}

/***********************************************************************
AddCVType
***********************************************************************/

Ptr<Type>& AddCVTypeInternal(Ptr<Type>& type, bool isConst, bool isVolatile)
{
	if (auto dt = type.Cast<DecorateType>())
	{
		type = dt->type;
		return AddCVTypeInternal(type, (isConst || dt->isConst), (isVolatile || dt->isVolatile));
	}
	else if (auto at = type.Cast<ArrayType>())
	{
		auto element = at->type;
		auto& core = AddCVTypeInternal(element, isConst, isVolatile);
		if (element == at->type)
		{
			if (&core == &element)
			{
				return at->type;
			}
			else
			{
				return core;
			}
		}
		else
		{
			auto newType = MakePtr<ArrayType>();
			newType->expr = at->expr;
			newType->type = element;
			
			type = newType;
			return core;
		}
	}
	else if (isConst || isVolatile)
	{
		auto newType = MakePtr<DecorateType>();
		newType->isConst = isConst;
		newType->isVolatile = isVolatile;
		newType->type = type;
		
		type = newType;
		return newType->type;
	}
	else
	{
		return type;
	}
}

Ptr<Type> AddCVType(Ptr<Type> type, bool isConst, bool isVolatile)
{
	if (!isConst && !isVolatile) return type;
	AddCVTypeInternal(type, isConst, isVolatile);
	return type;
}

/***********************************************************************
NormalizeTypeChain
***********************************************************************/

class NormalizeTypeChainTypeVisitor : public Object, public virtual ITypeVisitor
{
private:
	Ptr<Type>*						containerOfCurrentType = nullptr;

public:

	void Execute(Ptr<Type>& typeToNormalize)
	{
		if (typeToNormalize)
		{
			containerOfCurrentType = &typeToNormalize;
			typeToNormalize->Accept(this);
		}
	}

	void Visit(PrimitiveType* self)override
	{
	}

	void Visit(ReferenceType* self)override
	{
		Execute(self->type);
	}

	void Visit(ArrayType* self)override
	{
		auto& core = AddCVTypeInternal(*containerOfCurrentType, false, false);
		Execute(core);
	}

	void Visit(DecorateType* self)override
	{
		auto& core = AddCVTypeInternal(*containerOfCurrentType, false, false);
		Execute(core);
	}

	void Visit(CallingConventionType* self)override
	{
		Execute(self->type);
	}

	void Visit(FunctionType* self)override
	{
		Execute(self->returnType);
	}

	void Visit(MemberType* self)override
	{
		Execute(self->type);
	}

	void Visit(DeclType* self)override
	{
	}

	void Visit(RootType* self)override
	{
	}

	void Visit(IdType* self)override
	{
	}

	void Visit(ChildType* self)override
	{
	}

	void Visit(GenericType* self)override
	{
	}
};

void NormalizeTypeChain(Ptr<Type>& type)
{
	NormalizeTypeChainTypeVisitor().Execute(type);
}

/***********************************************************************
ParseCallingConvention
***********************************************************************/

bool ParseCallingConvention(TsysCallingConvention& callingConvention, Ptr<CppTokenCursor>& cursor)
{
	while (SkipSpecifiers(cursor));
#define CALLING_CONVENTION_KEYWORD(TOKEN, NAME)\
	if (TestToken(cursor, CppTokens::TOKEN))\
	{\
		callingConvention = TsysCallingConvention::NAME;\
		return true;\
	}\
	else\

	CALLING_CONVENTION_KEYWORD(__CDECL, CDecl)
	CALLING_CONVENTION_KEYWORD(__CLRCALL, ClrCall)
	CALLING_CONVENTION_KEYWORD(__STDCALL, StdCall)
	CALLING_CONVENTION_KEYWORD(__FASTCALL, FastCall)
	CALLING_CONVENTION_KEYWORD(__THISCALL, ThisCall)
	CALLING_CONVENTION_KEYWORD(__VECTORCALL, VectorCall)

#undef CALLING_CONVENTION_KEYWORD
	return false;
}