#include "Parser.h"
#include "Ast_Type.h"
#include "Ast_Decl.h"

/***********************************************************************
ReplaceOutOfDeclaratorTypeVisitor
***********************************************************************/

class ReplaceOutOfDeclaratorTypeVisitor : public Object, public virtual ITypeVisitor
{
public:
	Ptr<Type>						createdType;
	Ptr<Type>						typeToReplace;
	Func<Ptr<Type>(Ptr<Type>)>		typeCreator;

	void Execute(Ptr<Type>& targetType)
	{
		if (targetType == typeToReplace)
		{
			createdType = typeCreator(targetType);
			targetType = createdType;
		}
		else
		{
			targetType->Accept(this);
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
		Execute(self->type);
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

	void Visit(DecorateType* self)override
	{
		Execute(self->type);
	}

	void Visit(RootType* self)override
	{
	}

	void Visit(IdType* self)override
	{
	}

	void Visit(ChildType* self)override
	{
		Execute(self->classType);
	}

	void Visit(GenericType* self)override
	{
		Execute(self->type);
	}

	void Visit(VariadicTemplateArgumentType* self)override
	{
		Execute(self->type);
	}
};

/***********************************************************************
ParseShortDeclarator
***********************************************************************/

Ptr<Declarator> ParseShortDeclarator(const ParsingArguments& pa, Ptr<Type> typeResult, ClassDeclaration* containingClass, DeclaratorRestriction dr, Ptr<CppTokenCursor>& cursor)
{
	while (SkipSpecifiers(cursor));

	if (TestToken(cursor, CppTokens::ALIGNAS))
	{
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		ParseExpr(pa, false, cursor);
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		return ParseShortDeclarator(pa, typeResult, containingClass, dr, cursor);
	}
	else if (TestToken(cursor, CppTokens::MUL))
	{
		TestToken(cursor, CppTokens::__PTR32) || TestToken(cursor, CppTokens::__PTR64);
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::Ptr;
		type->type = typeResult;
		return ParseShortDeclarator(pa, type, containingClass, dr, cursor);
	}
	else if (TestToken(cursor, CppTokens::AND, CppTokens::AND))
	{
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::RRef;
		type->type = typeResult;
		return ParseShortDeclarator(pa, type, containingClass, dr, cursor);
	}
	else if (TestToken(cursor, CppTokens::AND))
	{
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::LRef;
		type->type = typeResult;
		return ParseShortDeclarator(pa, type, containingClass, dr, cursor);
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
		return ParseShortDeclarator(pa, type, containingClass, dr, cursor);
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
		return ParseShortDeclarator(pa, type, containingClass, dr, cursor);
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
		return ParseShortDeclarator(pa, type, containingClass, dr, cursor);
	}
	else
	{
		{
			auto oldCursor = cursor;
			CppCallingConvention callingConvention;
			if (ParseCallingConvention(callingConvention, cursor))
			{
				if (TestToken(cursor, CppTokens::LPARENTHESIS, false))
				{
					cursor = oldCursor;
				}
				else
				{
					auto type = MakePtr<CallingConventionType>();
					type->callingConvention = callingConvention;
					type->type = typeResult;
					return ParseShortDeclarator(pa, type, containingClass, dr, cursor);
				}
			}
		}

		Ptr<Type> classType;
		{
			auto oldCursor = cursor;
			try
			{
				classType = ParseLongType(pa, cursor);
				RequireToken(cursor, CppTokens::COLON, CppTokens::COLON);
			}
			catch (const StopParsingException&)
			{
				cursor = oldCursor;
				classType = nullptr;
			}
		}

		if (classType)
		{
			auto type = MakePtr<MemberType>();
			type->classType = classType;
			type->type = typeResult;
			return ParseShortDeclarator(pa, type, containingClass, dr, cursor);
		}
		else
		{
			auto declarator = MakePtr<Declarator>();
			declarator->type = typeResult;
			if (dr != DeclaratorRestriction::Zero)
			{
				if (ParseCppName(declarator->name, cursor, containingClass))
				{
					if (containingClass)
					{
						switch (declarator->name.type)
						{
						case CppNameType::Normal:
							if (declarator->name.name == containingClass->name.name)
							{
								declarator->name.name = L"$__ctor";
							}
							else
							{
								throw StopParsingException(cursor);
							}
							break;
						case CppNameType::Operator:
							if (declarator->name.tokenCount == 1)
							{
								declarator->name.name = L"$__type";
								auto type = ParseLongType(pa, cursor);
								if (ReplaceTypeInMemberAndCC(declarator->type, type))
								{
									throw StopParsingException(cursor);
								}
							}
							else
							{
								throw StopParsingException(cursor);
							}
							break;
						case CppNameType::Destructor:
							if (declarator->name.name != L"~" + containingClass->name.name)
							{
								throw StopParsingException(cursor);
							}
							break;
						}
					}
					else
					{
						switch (declarator->name.type)
						{
						case CppNameType::Operator:
							if (declarator->name.tokenCount == 1)
							{
								throw StopParsingException(cursor);
							}
							break;
						case CppNameType::Destructor:
							throw StopParsingException(cursor);
						}
					}
				}
				else
				{
					if (!TestToken(cursor, CppTokens::LPARENTHESIS, false) && dr == DeclaratorRestriction::One)
					{
						throw StopParsingException(cursor);
					}
				}
			}
			return declarator;
		}
	}
}

/***********************************************************************
ParseLongDeclarator
***********************************************************************/

Ptr<Declarator> ParseLongDeclarator(const ParsingArguments& pa, Ptr<Type> typeResult, ClassDeclaration* containingClass, DeclaratorRestriction dr, Ptr<CppTokenCursor>& cursor)
{
	auto declarator = ParseShortDeclarator(pa, typeResult, containingClass, dr, cursor);
	auto targetType = declarator->type;

	{
		auto oldCursor = cursor;
		if (!declarator->name)
		{
			if (TestToken(cursor, CppTokens::LPARENTHESIS) && TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				cursor = oldCursor;
				goto GIVE_UP;
			}

			cursor = oldCursor;
			if (TestToken(cursor, CppTokens::LPARENTHESIS) && TestToken(cursor, CppTokens::TYPE_VOID) && TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				cursor = oldCursor;
				goto GIVE_UP;
			}

			cursor = oldCursor;
			if (TestToken(cursor, CppTokens::LPARENTHESIS))
			{
				try
				{
					declarator = ParseLongDeclarator(pa, targetType, containingClass, dr, cursor);
				}
				catch (const StopParsingException&)
				{
					cursor = oldCursor;
					goto GIVE_UP;
				}

				RequireToken(cursor, CppTokens::RPARENTHESIS);
			}
		}

		if (!declarator->name && dr == DeclaratorRestriction::One)
		{
			throw StopParsingException(cursor);
		}
	}
GIVE_UP:

	if (TestToken(cursor, CppTokens::LBRACKET, false))
	{
		while (true)
		{
			if (TestToken(cursor, CppTokens::LBRACKET))
			{
				ReplaceOutOfDeclaratorTypeVisitor replacer;
				{
					replacer.typeToReplace = targetType;
					replacer.typeCreator = [](Ptr<Type> typeToReplace)
					{
						auto type = MakePtr<ArrayType>();
						type->type = typeToReplace;
						return type;
					};

					replacer.Execute(declarator->type);
					targetType = replacer.createdType;
				}

				if (!TestToken(cursor, CppTokens::RBRACKET))
				{
					replacer.createdType.Cast<ArrayType>()->expr = ParseExpr(pa, true, cursor);
					RequireToken(cursor, CppTokens::RBRACKET);
				}
			}
			else
			{
				return declarator;
			}
		}
	}

	bool hasCallingConvention = false;
	CppCallingConvention callingConvention;
	{
		auto oldCursor = cursor;
		if ((hasCallingConvention = ParseCallingConvention(callingConvention, cursor)))
		{
			if (!TestToken(cursor, CppTokens::LPARENTHESIS, false))
			{
				hasCallingConvention = false;
				cursor = oldCursor;
			}
		}
	}

	auto oldCursor = cursor;
	if (TestToken(cursor, CppTokens::LPARENTHESIS))
	{
		try
		{
			ParseExpr(pa, false, cursor);
			cursor = oldCursor;
			return declarator;
		}
		catch (const StopParsingException&)
		{
			cursor = oldCursor->Next();
		}

		Ptr<FunctionType> type;
		if (hasCallingConvention)
		{
			if (declarator->type != targetType)
			{
				throw StopParsingException(cursor);
			}

			type = MakePtr<FunctionType>();
			type->returnType = declarator->type;

			auto ccType = MakePtr<CallingConventionType>();
			ccType->callingConvention = callingConvention;
			ccType->type = type;
			declarator->type = ccType;
		}
		else
		{
			ReplaceOutOfDeclaratorTypeVisitor replacer;
			{
				replacer.typeToReplace = targetType;
				replacer.typeCreator = [](Ptr<Type> typeToReplace)->Ptr<Type>
				{
					auto type = MakePtr<FunctionType>();
					type->returnType = typeToReplace;
					return AdjustReturnTypeWithMemberAndCC(type);
				};

				replacer.Execute(declarator->type);
			}
			type = GetTypeWithoutMemberAndCC(replacer.createdType).Cast<FunctionType>();
		}

		{
			auto oldCursor = cursor;
			if (TestToken(cursor, CppTokens::TYPE_VOID) && TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				goto FINISH_PARAMETER_LIST;
			}
			else
			{
				cursor = oldCursor;
			}
		}

		while (!TestToken(cursor, CppTokens::RPARENTHESIS))
		{
			{
				List<Ptr<Declarator>> declarators;
				ParseDeclarator(pa, nullptr, DeclaratorRestriction::Optional, InitializerRestriction::Optional, cursor, declarators);
				List<Ptr<VariableDeclaration>> varDecls;
				BuildVariablesAndSymbols(pa, declarators, varDecls, false);
				type->parameters.Add(varDecls[0]);
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
	FINISH_PARAMETER_LIST:

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
				ParseDeclarator(pa, nullptr, DeclaratorRestriction::Zero, InitializerRestriction::Zero, cursor, declarators);
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
					ParseDeclarator(pa, nullptr, DeclaratorRestriction::Zero, InitializerRestriction::Zero, cursor, declarators);
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
				return declarator;
			}
		}
	}

	return declarator;
}

/***********************************************************************
ParseInitializer
***********************************************************************/

Ptr<Initializer> ParseInitializer(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
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
	else
	{
		throw StopParsingException(cursor);
	}

	while (true)
	{
		initializer->arguments.Add(ParseExpr(pa, false, cursor));

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

/***********************************************************************
ParseArrayFunctionDeclarator
***********************************************************************/

void ParseArrayFunctionDeclarator(const ParsingArguments& pa, Ptr<Type> typeResult, ClassDeclaration* containingClass, DeclaratorRestriction dr, InitializerRestriction ir, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators)
{
	auto itemDr = dr == DeclaratorRestriction::Many ? DeclaratorRestriction::One : dr;
	while (true)
	{
		auto declarator = ParseLongDeclarator(pa, typeResult, containingClass, itemDr, cursor);

		if (containingClass)
		{
			if (!GetTypeWithoutMemberAndCC(declarator->type).Cast<FunctionType>())
			{
				throw StopParsingException(cursor);
			}
		}

		if (ir == InitializerRestriction::Optional)
		{
			bool isFunction = GetTypeWithoutMemberAndCC(declarator->type).Cast<FunctionType>();
			if (TestToken(cursor, CppTokens::EQ, false) || TestToken(cursor, CppTokens::LPARENTHESIS, false))
			{
				declarator->initializer = ParseInitializer(pa, cursor);
			}
			else if (TestToken(cursor, CppTokens::LBRACE, false))
			{
				auto oldCursor = cursor;
				try
				{
					declarator->initializer = ParseInitializer(pa, cursor);
				}
				catch (const StopParsingException&)
				{
					if (isFunction)
					{
						cursor = oldCursor;
					}
					else
					{
						throw;
					}
				}
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

/***********************************************************************
ParseDeclarator
***********************************************************************/

void ParseDeclarator(const ParsingArguments& pa, ClassDeclaration* containingClass, DeclaratorRestriction dr, InitializerRestriction ir, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators)
{
	Ptr<Type> typeResult = ParseLongType(pa, cursor);
	return ParseArrayFunctionDeclarator(pa, typeResult, containingClass, dr, ir, cursor, declarators);
}