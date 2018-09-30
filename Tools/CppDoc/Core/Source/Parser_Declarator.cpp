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

ClassDeclaration* EnsureMemberTypeResolved(Ptr<MemberType> memberType, Ptr<CppTokenCursor>& cursor)
{
	auto resolvableType = memberType->classType.Cast<ResolvableType>();
	if (!resolvableType) throw StopParsingException(cursor);
	if (!resolvableType->resolving) throw StopParsingException(cursor);
	resolvableType->resolving->Calibrate();
	if (resolvableType->resolving->resolvedSymbols.Count() != 1) throw StopParsingException(cursor);

	auto symbol = resolvableType->resolving->resolvedSymbols[0];
	if (!symbol->decls.Count() == 1) throw StopParsingException(cursor);
	auto containingClass = symbol->decls[0].Cast<ClassDeclaration>().Obj();
	if (!containingClass) throw StopParsingException(cursor);
	return containingClass;
}

/***********************************************************************
ParseShortDeclarator
***********************************************************************/

Ptr<Declarator> ParseShortDeclarator(const ParsingArguments& pa, Ptr<Type> typeResult, ClassDeclaration* containingClass, bool forceSpecialMethod, DeclaratorRestriction dr, Ptr<CppTokenCursor>& cursor)
{
	while (SkipSpecifiers(cursor));

	if (TestToken(cursor, CppTokens::ALIGNAS))
	{
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		ParseExpr(pa, false, cursor);
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		return ParseShortDeclarator(pa, typeResult, containingClass, forceSpecialMethod, dr, cursor);
	}
	else if (TestToken(cursor, CppTokens::MUL))
	{
		TestToken(cursor, CppTokens::__PTR32) || TestToken(cursor, CppTokens::__PTR64);
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::Ptr;
		type->type = typeResult;
		return ParseShortDeclarator(pa, type, containingClass, forceSpecialMethod, dr, cursor);
	}
	else if (TestToken(cursor, CppTokens::AND, CppTokens::AND))
	{
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::RRef;
		type->type = typeResult;
		return ParseShortDeclarator(pa, type, containingClass, forceSpecialMethod, dr, cursor);
	}
	else if (TestToken(cursor, CppTokens::AND))
	{
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::LRef;
		type->type = typeResult;
		return ParseShortDeclarator(pa, type, containingClass, forceSpecialMethod, dr, cursor);
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
		return ParseShortDeclarator(pa, type, containingClass, forceSpecialMethod, dr, cursor);
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
		return ParseShortDeclarator(pa, type, containingClass, forceSpecialMethod, dr, cursor);
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
		return ParseShortDeclarator(pa, type, containingClass, forceSpecialMethod, dr, cursor);
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
					return ParseShortDeclarator(pa, type, containingClass, forceSpecialMethod, dr, cursor);
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

			if(forceSpecialMethod && !containingClass)
			{
				auto oldCursor = cursor;
				CppName cppName;
				if (ParseCppName(cppName, cursor, true))
				{
					containingClass = EnsureMemberTypeResolved(type, cursor);
				}
				cursor = oldCursor;
			}
			return ParseShortDeclarator(pa, type, containingClass, forceSpecialMethod, dr, cursor);
		}
		else
		{
			auto declarator = MakePtr<Declarator>();
			declarator->type = typeResult;
			if (containingClass)
			{
				declarator->containingClassSymbol = containingClass->symbol;
			}
			else if (auto memberType = typeResult.Cast<MemberType>())
			{
				declarator->containingClassSymbol = EnsureMemberTypeResolved(memberType, cursor)->symbol;
			}

			if (dr != DeclaratorRestriction::Zero)
			{
				if (ParseCppName(declarator->name, cursor, forceSpecialMethod))
				{
					if (forceSpecialMethod)
					{
						if (!containingClass)
						{
							throw StopParsingException(cursor);
						}

						switch (declarator->name.type)
						{
						case CppNameType::Normal:
							if (declarator->name.name == containingClass->name.name)
							{
								declarator->name.name = L"$__ctor";
								declarator->name.type = CppNameType::Constructor;
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
						case CppNameType::Constructor:
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

Ptr<Declarator> ParseLongDeclarator(const ParsingArguments& pa, Ptr<Type> typeResult, ClassDeclaration* containingClass, bool forceSpecialMethod, DeclaratorRestriction dr, Ptr<CppTokenCursor>& cursor)
{
	auto declarator = ParseShortDeclarator(pa, typeResult, containingClass, forceSpecialMethod, dr, cursor);
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
					declarator = ParseLongDeclarator(pa, targetType, containingClass, forceSpecialMethod, dr, cursor);
					RequireToken(cursor, CppTokens::RPARENTHESIS);
				}
				catch (const StopParsingException&)
				{
					cursor = oldCursor;
					goto GIVE_UP;
				}
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

		{
			ParsingArguments functionArgsPa = pa;
			if (declarator->type.Cast<MemberType>() && declarator->containingClassSymbol)
			{
				functionArgsPa.context = declarator->containingClassSymbol;
			}
			while (!TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				{
					List<Ptr<Declarator>> declarators;
					ParseDeclarator(functionArgsPa, nullptr, false, DeclaratorRestriction::Optional, InitializerRestriction::Optional, cursor, declarators);
					List<Ptr<VariableDeclaration>> varDecls;
					BuildVariables(declarators, varDecls);
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

				type->decoratorReturnType = ParseType(pa, cursor);
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
					type->exceptions.Add(ParseType(pa, cursor));

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
ParseDeclaratorWithInitializer
***********************************************************************/

void ParseDeclaratorWithInitializer(const ParsingArguments& pa, Ptr<Type> typeResult, ClassDeclaration* containingClass, bool forceSpecialMethod, DeclaratorRestriction dr, InitializerRestriction ir, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators)
{
	auto itemDr = dr == DeclaratorRestriction::Many ? DeclaratorRestriction::One : dr;
	while (true)
	{
		auto declarator = ParseLongDeclarator(pa, typeResult, containingClass, forceSpecialMethod, itemDr, cursor);

		ParsingArguments initializerPa = pa;
		if (declarator->type.Cast<MemberType>() && declarator->containingClassSymbol)
		{
			initializerPa.context = declarator->containingClassSymbol;
		}

		if (ir == InitializerRestriction::Optional)
		{
			bool isFunction = GetTypeWithoutMemberAndCC(declarator->type).Cast<FunctionType>();
			if (TestToken(cursor, CppTokens::EQ, false) || TestToken(cursor, CppTokens::LPARENTHESIS, false))
			{
				declarator->initializer = ParseInitializer(initializerPa, cursor);
			}
			else if (TestToken(cursor, CppTokens::LBRACE, false))
			{
				auto oldCursor = cursor;
				try
				{
					declarator->initializer = ParseInitializer(initializerPa, cursor);
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

void ParseDeclarator(const ParsingArguments& pa, ClassDeclaration* containingClass, bool trySpecialMember, DeclaratorRestriction dr, InitializerRestriction ir, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators)
{
	if (trySpecialMember && dr == DeclaratorRestriction::Many)
	{
		auto oldCursor = cursor;
		{
			try
			{
				ParseDeclaratorWithInitializer(pa, nullptr, containingClass, true, DeclaratorRestriction::One, ir, cursor, declarators);
			}
			catch (const StopParsingException&)
			{
				goto TRY_NORMAL_DECLARATOR;
			}

			if (declarators.Count() == 0)
			{
				goto TRY_NORMAL_DECLARATOR;
			}
			for (vint i = 0; i < declarators.Count(); i++)
			{
				if (!declarators[i]->type)
				{
					declarators.Clear();
					goto TRY_NORMAL_DECLARATOR;
				}
			}

			return;
		}

	TRY_NORMAL_DECLARATOR:
		cursor = oldCursor;
	}

	auto typeResult = ParseLongType(pa, cursor);
	ParseDeclaratorWithInitializer(pa, typeResult, containingClass, false, dr, ir, cursor, declarators);
}

/***********************************************************************
ParseDeclarator
***********************************************************************/

void ParseNonMemberDeclarator(const ParsingArguments& pa, DeclaratorRestriction dr, InitializerRestriction ir, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators)
{
	ParseDeclarator(pa, nullptr, false, dr, ir, cursor, declarators);
}

/***********************************************************************
ParseDeclarator
***********************************************************************/

Ptr<Type> ParseType(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	List<Ptr<Declarator>> declarators;
	ParseNonMemberDeclarator(pa, DeclaratorRestriction::Zero, InitializerRestriction::Zero, cursor, declarators);
	return declarators[0]->type;
}