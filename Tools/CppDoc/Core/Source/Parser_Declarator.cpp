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
		// alignas (EXPRESSION) DECLARATOR
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		ParseExpr(pa, false, cursor);
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		return ParseShortDeclarator(pa, typeResult, containingClass, forceSpecialMethod, dr, cursor);
	}
	else if (TestToken(cursor, CppTokens::MUL))
	{
		// * DECLARATOR
		// * __ptr32 DECLARATOR
		// * __ptr64 DECLARATOR
		TestToken(cursor, CppTokens::__PTR32) || TestToken(cursor, CppTokens::__PTR64);
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::Ptr;
		type->type = typeResult;
		return ParseShortDeclarator(pa, type, containingClass, forceSpecialMethod, dr, cursor);
	}
	else if (TestToken(cursor, CppTokens::AND, CppTokens::AND))
	{
		// && DECLARATOR
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::RRef;
		type->type = typeResult;
		return ParseShortDeclarator(pa, type, containingClass, forceSpecialMethod, dr, cursor);
	}
	else if (TestToken(cursor, CppTokens::AND))
	{
		// & DECLARATOR
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::LRef;
		type->type = typeResult;
		return ParseShortDeclarator(pa, type, containingClass, forceSpecialMethod, dr, cursor);
	}
	else if (TestToken(cursor, CppTokens::CONSTEXPR))
	{
		// constexpr DECLARATOR
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
		// const DECLARATOR
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
		// volatile DECLARATOR
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
					// for __stdcall (
					// because there is no __stdcall (DECLARATOR), so it can only be __stdcall ( { PARAMETER ,...} )
					// so this __stdcall will be consumed by ParseLongDeclarator, so quit here
					// if any declarator name is required, an exception will be raised below
					cursor = oldCursor;
				}
				else
				{
					// __stdcall DECLARATOR
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
			// CLASS:: DECLARATOR
			auto type = MakePtr<MemberType>();
			type->classType = classType;
			type->type = typeResult;

			if(forceSpecialMethod && !containingClass)
			{
				// for CLASS:: NAME
				// ensure CLASS is resolved to a single type
				// and a CLASS::NAME declaration cannot appear inside a class, so in this case we skip the test
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
			// NAME
			auto declarator = MakePtr<Declarator>();
			declarator->type = typeResult;

			// recognize a class member declaration
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
				// forceSpecialMethod means this function is expected to accept only
				//   constructor declarators
				//   destructor declarators
				//   type conversion declarators
				// the common thing among them is not having a return type before the declarator
				if (ParseCppName(declarator->name, cursor, forceSpecialMethod))
				{
					if (forceSpecialMethod)
					{
						// special method can only appear
						//   when a declaration is right inside a class
						//   or it is a class member declaration out of a class
						if (!containingClass)
						{
							throw StopParsingException(cursor);
						}

						switch (declarator->name.type)
						{
						case CppNameType::Normal:
							// IDENTIFIER should be a constructor name for a special method
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
							// operator TYPE is the only valid form of special method if the first token is operator
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
							// ~IDENTIFIER should be a destructor name for a special method
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
							// operator PREDEFINED-OPERATOR is the only valid form for normal method if the first token is operator
							if (declarator->name.tokenCount == 1)
							{
								throw StopParsingException(cursor);
							}
							break;
						case CppNameType::Constructor:
						case CppNameType::Destructor:
							// constructors or destructors are not normal methods
							throw StopParsingException(cursor);
						}
					}
				}
				else
				{
					// we have already tested all possible form of declarators
					// if a declarator name is required, there are only two possibilities left
					//    1. the declarator name is placed after (
					//    2. syntax error
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
	// a long declarator begins with a short declarator
	auto declarator = ParseShortDeclarator(pa, typeResult, containingClass, forceSpecialMethod, dr, cursor);
	auto targetType = declarator->type;

	{
		auto oldCursor = cursor;
		if (!declarator->name)
		{
			// if there is no declarator name, and the next token is (
			// then a declarator is possible to be placed after (, and it is also possible that this declarator still doesn't have a name

			// but this is not the case when we see ()
			if (TestToken(cursor, CppTokens::LPARENTHESIS) && TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				cursor = oldCursor;
				goto GIVE_UP;
			}

			// but this is not the case when we see (void)
			cursor = oldCursor;
			if (TestToken(cursor, CppTokens::LPARENTHESIS) && TestToken(cursor, CppTokens::TYPE_VOID) && TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				cursor = oldCursor;
				goto GIVE_UP;
			}

			// try to get a declarator name, if failed then we ignore and we assume there are parameters after (
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

	// if there is [, we see an array declarator
	// an array could be multiple dimension
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
				// there is no something like []( { PARAMETERS, ...} ), so we can stop here
				return declarator;
			}
		}
	}

	// if it is not an array declarator, then there are only two possibilities
	//   1. it is a function declarator
	//   2. it is a declarator but not array or function
	// so we see if we can find __stdcall
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
		// if we see (EXPRESSION, then this is an initializer, we stop here
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
			// __stdcall should appear before CLASS::
			// so if we see __stdcall ( here, then there is no CLASS:: (maybe CLASS::* but we don't care)
			// we can simply decorate the function with __stdcall, no need to worry about adjusting the return type
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
			// otherwise, it is possible that the return type has already been decorated with __stdcall or CLASS::
			// we should move them out of the return type, and decorate the function type with them
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
		// here type is the FunctionType we are going to add information to, it is possible that type is not declarator->type

		{
			// (void) is treated as no parameter
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
			// if there is CLASS::, then we should regonize parameter types under the scope of CLASS
			ParsingArguments functionArgsPa = pa;
			if (declarator->type.Cast<MemberType>() && declarator->containingClassSymbol)
			{
				functionArgsPa.context = declarator->containingClassSymbol;
			}

			// recognize parameters
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
			// we can put these things after a function declarator, they decorate *this, not any type inside the function
			//   constexpr
			//   const
			//   volatile
			//   &&
			//   &
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
				// override
				type->decoratorOverride = true;
			}
			else if (TestToken(cursor, CppTokens::SUB, CppTokens::GT))
			{
				// auto SOMETHING -> TYPE
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
				// noexcept
				type->decoratorNoExcept = true;
			}
			else if (TestToken(cursor, CppTokens::THROW))
			{
				// throw ( { TYPE , ...} )
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
	// = EXPRESSION
	// { { EXPRESSION , ...} }
	// ( { EXPRESSSION , ...} )
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
	// if we have already recognize a type, we can parse multiple declarators with initializers
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
						// { could be the beginning of a statement
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
		// if we don't see trySpecialMember or Many, then the declarator is not for a declaration, so there is no special method
		auto oldCursor = cursor;
		{
			try
			{
				// try to parse a special method declarator
				ParseDeclaratorWithInitializer(pa, nullptr, containingClass, true, DeclaratorRestriction::One, ir, cursor, declarators);
			}
			catch (const StopParsingException&)
			{
				// ignore it if we failed
				goto TRY_NORMAL_DECLARATOR;
			}

			if (declarators.Count() == 0)
			{
				goto TRY_NORMAL_DECLARATOR;
			}
			for (vint i = 0; i < declarators.Count(); i++)
			{
				// the type of a special method should be a (maybe decorated) function, but its return type could be nullptr
				// so if the type is nullptr, then we are definitely failed
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