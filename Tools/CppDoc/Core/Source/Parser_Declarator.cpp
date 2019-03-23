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
};

/***********************************************************************
EnsureMemberTypeResolved
***********************************************************************/

ClassDeclaration* EnsureMemberTypeResolved(Ptr<MemberType> memberType, Ptr<CppTokenCursor>& cursor)
{
	auto resolvableType = memberType->classType.Cast<ResolvableType>();
	if (!resolvableType) throw StopParsingException(cursor);
	if (!resolvableType->resolving) throw StopParsingException(cursor);
	if (resolvableType->resolving->resolvedSymbols.Count() != 1) throw StopParsingException(cursor);

	auto symbol = resolvableType->resolving->resolvedSymbols[0];
	auto containingClass = symbol->definition.Cast<ClassDeclaration>().Obj();
	if (!containingClass) throw StopParsingException(cursor);
	return containingClass;
}

/***********************************************************************
ParseDeclaratorContext
***********************************************************************/

struct ParseDeclaratorContext
{
	ClassDeclaration*		containingClass;
	bool					forParameter;
	DeclaratorRestriction	dr;
	InitializerRestriction	ir;
	bool					allowBitField;
	bool					forceSpecialMethod;

	ParseDeclaratorContext(const ParsingDeclaratorArguments& pda, bool _forceSpecialMethod)
		:containingClass(pda.containingClass)
		, forParameter(pda.forParameter)
		, dr(pda.dr)
		, ir(pda.ir)
		, allowBitField(pda.allowBitField)
		, forceSpecialMethod(_forceSpecialMethod)
	{
	}
};

/***********************************************************************
ParseDeclaratorName
***********************************************************************/

bool ParseDeclaratorName(const ParsingArguments& pa, CppName& cppName, Ptr<Type>& targetType, const ParseDeclaratorContext& pdc, Ptr<CppTokenCursor>& cursor)
{
	// forceSpecialMethod means this function is expected to accept only
	//   constructor declarators
	//   destructor declarators
	//   type conversion declarators
	// the common thing among them is not having a return type before the declarator
	if (ParseCppName(cppName, cursor, pdc.forceSpecialMethod))
	{
		if (pdc.forceSpecialMethod)
		{
			// special method can only appear
			//   when a declaration is right inside a class
			//   or it is a class member declaration out of a class

			auto containingClass = pdc.containingClass;
			if (!containingClass)
			{
				if (auto memberType = targetType.Cast<MemberType>())
				{
					containingClass = EnsureMemberTypeResolved(memberType, cursor);
				}
			}
			if (!containingClass)
			{
				throw StopParsingException(cursor);
			}

			switch (cppName.type)
			{
			case CppNameType::Normal:
				// IDENTIFIER should be a constructor name for a special method
				if (cppName.name == containingClass->name.name)
				{
					cppName.name = L"$__ctor";
					cppName.type = CppNameType::Constructor;
				}
				else
				{
					throw StopParsingException(cursor);
				}
				break;
			case CppNameType::Operator:
				// operator TYPE is the only valid form of special method if the first token is operator
				if (cppName.tokenCount == 1)
				{
					cppName.name = L"$__type";
					auto type = ParseLongType(pa, cursor);
					if (ReplaceTypeInMemberAndCC(targetType, type))
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
				if (cppName.name != L"~" + containingClass->name.name)
				{
					throw StopParsingException(cursor);
				}
				break;
			}
		}
		else
		{
			switch (cppName.type)
			{
			case CppNameType::Operator:
				// operator PREDEFINED-OPERATOR is the only valid form for normal method if the first token is operator
				if (cppName.tokenCount == 1)
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
		return true;
	}
	else
	{
		return false;
	}
}

/***********************************************************************
ParseTypeBeforeDeclarator
***********************************************************************/

Ptr<Type> ParseTypeBeforeDeclarator(const ParsingArguments& pa, Ptr<Type> baselineType, const ParseDeclaratorContext& pdc, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::ALIGNAS))
	{
		// alignas (EXPRESSION) DECLARATOR
		RequireToken(cursor, CppTokens::LPARENTHESIS);
		ParseExpr(pa, pea_Argument(), cursor);
		RequireToken(cursor, CppTokens::RPARENTHESIS);
		return ParseTypeBeforeDeclarator(pa, baselineType, pdc, cursor);
	}
	else if (TestToken(cursor, CppTokens::MUL))
	{
		// * DECLARATOR
		// * __ptr32 DECLARATOR
		// * __ptr64 DECLARATOR
		TestToken(cursor, CppTokens::__PTR32) || TestToken(cursor, CppTokens::__PTR64);
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::Ptr;
		type->type = baselineType;
		return ParseTypeBeforeDeclarator(pa, type, pdc, cursor);
	}
	else if (TestToken(cursor, CppTokens::AND, CppTokens::AND))
	{
		// && DECLARATOR
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::RRef;
		type->type = baselineType;
		return ParseTypeBeforeDeclarator(pa, type, pdc, cursor);
	}
	else if (TestToken(cursor, CppTokens::AND))
	{
		// & DECLARATOR
		auto type = MakePtr<ReferenceType>();
		type->reference = CppReferenceType::LRef;
		type->type = baselineType;
		return ParseTypeBeforeDeclarator(pa, type, pdc, cursor);
	}
	else if (TestToken(cursor, CppTokens::CONSTEXPR))
	{
		// constexpr DECLARATOR
		auto type = baselineType.Cast<DecorateType>();
		if (!type)
		{
			type = MakePtr<DecorateType>();
			type->type = baselineType;
		}
		type->isConstExpr = true;
		return ParseTypeBeforeDeclarator(pa, type, pdc, cursor);
	}
	else if (TestToken(cursor, CppTokens::CONST))
	{
		// const DECLARATOR
		auto type = baselineType.Cast<DecorateType>();
		if (!type)
		{
			type = MakePtr<DecorateType>();
			type->type = baselineType;
		}
		type->isConst = true;
		return ParseTypeBeforeDeclarator(pa, type, pdc, cursor);
	}
	else if (TestToken(cursor, CppTokens::VOLATILE))
	{
		// volatile DECLARATOR
		auto type = baselineType.Cast<DecorateType>();
		if (!type)
		{
			type = MakePtr<DecorateType>();
			type->type = baselineType;
		}
		type->isVolatile = true;
		return ParseTypeBeforeDeclarator(pa, type, pdc, cursor);
	}
	else
	{
		{
			auto oldCursor = cursor;
			TsysCallingConvention callingConvention;
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
					type->type = baselineType;
					return ParseTypeBeforeDeclarator(pa, type, pdc, cursor);
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
			type->type = baselineType;
			return ParseTypeBeforeDeclarator(pa, type, pdc, cursor);
		}
		else
		{
			return baselineType;
		}
	}
}

/***********************************************************************
ParseSingleDeclarator_Array
***********************************************************************/

bool ParseSingleDeclarator_Array(const ParsingArguments& pa, Ptr<Declarator> declarator, Ptr<Type> targetType, bool forParameter, Ptr<CppTokenCursor>& cursor)
{
	if (TestToken(cursor, CppTokens::LBRACKET))
	{
		Ptr<Expr> index;
		if (!TestToken(cursor, CppTokens::RBRACKET, false))
		{
			index = ParseExpr(pa, pea_Full(), cursor);
		}

		ReplaceOutOfDeclaratorTypeVisitor replacer;
		{
			replacer.typeToReplace = targetType;
			replacer.typeCreator = [=](Ptr<Type> typeToReplace)->Ptr<Type>
			{
				if (index || !forParameter)
				{
					auto type = MakePtr<ArrayType>();
					type->type = typeToReplace;
					type->expr = index;
					return type;
				}
				else
				{
					auto type = MakePtr<ReferenceType>();
					type->reference = CppReferenceType::Ptr;
					type->type = typeToReplace;
					return type;
				}
			};

			replacer.Execute(declarator->type);
		}

		RequireToken(cursor, CppTokens::RBRACKET);
		return true;
	}
	else
	{
		// there is no something like []( { PARAMETERS, ...} ), so we can stop here
		return false;
	}
}

/***********************************************************************
ParseSingleDeclarator_Function
***********************************************************************/

bool ParseSingleDeclarator_Function(const ParsingArguments& pa, Ptr<Declarator> declarator, Ptr<Type> targetType, bool forceSpecialMethod, Ptr<CppTokenCursor>& cursor)
{
	// if it is not an array declarator, then there are only two possibilities
	//   1. it is a function declarator
	//   2. it is a declarator but not array or function
	// so we see if we can find __stdcall
	auto callingConvention = TsysCallingConvention::None;
	{
		auto oldCursor = cursor;
		if (ParseCallingConvention(callingConvention, cursor))
		{
			if (!TestToken(cursor, CppTokens::LPARENTHESIS, false))
			{
				cursor = oldCursor;
			}
		}
	}

	if (TestToken(cursor, CppTokens::LPARENTHESIS, false))
	{
		if (!forceSpecialMethod || callingConvention != TsysCallingConvention::None)
		{
			// if we see (EXPRESSION, then this is an initializer, we stop here.
			// for special method, it should be a function, so we don't need to test for an initializer.
			// and there is also not possible to have an expression right after __stdcall.
			auto oldCursor = cursor;
			try
			{
				SkipToken(cursor);
				ParseExpr(pa, pea_Argument(), cursor);
				cursor = oldCursor;
				return true;
			}
			catch (const StopParsingException&)
			{
				cursor = oldCursor;
			}
		}

		SkipToken(cursor);
		Ptr<FunctionType> type;
		if (callingConvention != TsysCallingConvention::None)
		{
			// __stdcall should appear before "CLASS::"
			// so if we see "__stdcall (" here, then there is no "CLASS::" (maybe "CLASS::*" but we don't care)
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
			if (!TestToken(cursor, CppTokens::RPARENTHESIS))
			{
				while (true)
				{
					if (TestToken(cursor, CppTokens::DOT, CppTokens::DOT, CppTokens::DOT))
					{
						RequireToken(cursor, CppTokens::RPARENTHESIS);
						type->ellipsis = true;
						break;
					}

					{
						List<Ptr<Declarator>> declarators;
						while (SkipSpecifiers(cursor));
						ParseNonMemberDeclarator(functionArgsPa, pda_Param(true), cursor, declarators);
						List<Ptr<VariableDeclaration>> varDecls;
						BuildVariables(declarators, varDecls);
						type->parameters.Add(varDecls[0]);
					}

					if (!TestToken(cursor, CppTokens::COMMA))
					{
						RequireToken(cursor, CppTokens::RPARENTHESIS);
						break;
					}
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
				if (!TestToken(cursor, CppTokens::RPARENTHESIS))
				{
					while (true)
					{
						type->exceptions.Add(ParseType(pa, cursor));

						if (!TestToken(cursor, CppTokens::COMMA))
						{
							RequireToken(cursor, CppTokens::RPARENTHESIS);
							break;
						}
					}
				}
			}
			else
			{
				return true;
			}
		}
	}
	else
	{
		return false;
	}
}

/***********************************************************************
ParseSingleDeclarator
***********************************************************************/

Ptr<Declarator> ParseSingleDeclarator(const ParsingArguments& pa, Ptr<Type> baselineType, const ParseDeclaratorContext& pdc, Ptr<CppTokenCursor>& cursor)
{
	Ptr<Declarator> declarator;

	// a long declarator begins with more type decorations
	auto targetType = ParseTypeBeforeDeclarator(pa, baselineType, pdc, cursor);

	if (pdc.dr != DeclaratorRestriction::Zero)
	{
		while (SkipSpecifiers(cursor));

		// there may be a declarator name
		CppName cppName;
		if (ParseDeclaratorName(pa, cppName, targetType, pdc, cursor))
		{
			declarator = new Declarator;
			declarator->type = targetType;
			declarator->name = cppName;
		}
	}

	if (pdc.allowBitField && TestToken(cursor, CppTokens::COLON))
	{
		// bit fields, just ignore
		ParseExpr(pa, pea_Argument(), cursor);
	}

	if (!declarator)
	{
		auto oldCursor = cursor;
		// if there is no declarator name, and the next token is (
		// then a declarator is possible to be placed after (, and it is also possible that this declarator still doesn't have a name

		// but this is not the case when we see ()
		if (TestToken(cursor, CppTokens::LPARENTHESIS) && TestToken(cursor, CppTokens::RPARENTHESIS))
		{
			cursor = oldCursor;
			goto READY_FOR_ARRAY_OR_FUNCTION;
		}

		// but this is not the case when we see (void)
		cursor = oldCursor;
		if (TestToken(cursor, CppTokens::LPARENTHESIS) && TestToken(cursor, CppTokens::TYPE_VOID) && TestToken(cursor, CppTokens::RPARENTHESIS))
		{
			cursor = oldCursor;
			goto READY_FOR_ARRAY_OR_FUNCTION;
		}

		// try to get a declarator name, if failed then we ignore and we assume there are parameters after (
		cursor = oldCursor;
		if (TestToken(cursor, CppTokens::LPARENTHESIS))
		{
			try
			{
				declarator = ParseSingleDeclarator(pa, targetType, pdc, cursor);
				RequireToken(cursor, CppTokens::RPARENTHESIS);
			}
			catch (const StopParsingException&)
			{
				cursor = oldCursor;
				goto READY_FOR_ARRAY_OR_FUNCTION;
			}
		}
	}

READY_FOR_ARRAY_OR_FUNCTION:
	if (!declarator)
	{
		declarator = new Declarator;
		declarator->type = targetType;
	}

	// check if we have already done with the declarator name
	if (!declarator->name && pdc.dr == DeclaratorRestriction::One)
	{
		throw StopParsingException(cursor);
	}

	// recognize a class member declaration
	if (pdc.containingClass)
	{
		declarator->containingClassSymbol = pdc.containingClass->symbol;
	}
	else if (auto memberType = declarator->type.Cast<MemberType>())
	{
		declarator->containingClassSymbol = EnsureMemberTypeResolved(memberType, cursor)->symbol;
	}

	// if there is [, we see an array declarator
	// an array could be multiple dimension
	auto newPa = declarator->containingClassSymbol ? pa.WithContext(declarator->containingClassSymbol) : pa;
	if (TestToken(cursor, CppTokens::LBRACKET, false))
	{
		while (ParseSingleDeclarator_Array(newPa, declarator, targetType, pdc.forParameter, cursor));
	}
	else
	{
		ParseSingleDeclarator_Function(newPa, declarator, targetType, pdc.forceSpecialMethod, cursor);
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
		initializer->initializerType = CppInitializerType::Equal;
	}
	else if (TestToken(cursor, CppTokens::LBRACE))
	{
		initializer->initializerType = CppInitializerType::Universal;
	}
	else if (TestToken(cursor, CppTokens::LPARENTHESIS))
	{
		initializer->initializerType = CppInitializerType::Constructor;
	}
	else
	{
		throw StopParsingException(cursor);
	}

	while (true)
	{
		initializer->arguments.Add(ParseExpr(pa, pea_Argument(), cursor));

		if (initializer->initializerType == CppInitializerType::Equal)
		{
			break;
		}
		else if (!TestToken(cursor, CppTokens::COMMA))
		{
			switch (initializer->initializerType)
			{
			case CppInitializerType::Universal:
				RequireToken(cursor, CppTokens::RBRACE);
				break;
			case CppInitializerType::Constructor:
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

void ParseDeclaratorWithInitializer(const ParsingArguments& pa, Ptr<Type> typeResult, const ParseDeclaratorContext& pdc, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators)
{
	// if we have already recognize a type, we can parse multiple declarators with initializers
	auto newPdc = pdc;
	newPdc.dr = pdc.dr == DeclaratorRestriction::Many ? DeclaratorRestriction::One : pdc.dr;

	while (true)
	{
		auto declarator = ParseSingleDeclarator(pa, typeResult, newPdc, cursor);

		ParsingArguments initializerPa = pa;
		if (declarator->type.Cast<MemberType>() && declarator->containingClassSymbol)
		{
			initializerPa.context = declarator->containingClassSymbol;
		}

		// function doesn't have initializer
		bool isFunction = GetTypeWithoutMemberAndCC(declarator->type).Cast<FunctionType>();
		if (pdc.ir == InitializerRestriction::Optional && !isFunction)
		{
			if (TestToken(cursor, CppTokens::EQ, false) || TestToken(cursor, CppTokens::LPARENTHESIS, false))
			{
				declarator->initializer = ParseInitializer(initializerPa, cursor);
			}
			else if (TestToken(cursor, CppTokens::LBRACE, false))
			{
				declarator->initializer = ParseInitializer(initializerPa, cursor);
			}
		}
		declarators.Add(declarator);

		if (pdc.dr != DeclaratorRestriction::Many)
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

void ParseDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, bool trySpecialMember, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators)
{
	if (trySpecialMember && pda.dr == DeclaratorRestriction::Many)
	{
		// if we don't see trySpecialMember or Many, then the declarator is not for a declaration, so there is no special method
		auto oldCursor = cursor;
		{
			try
			{
				// try to parse a special method declarator
				ParseDeclaratorWithInitializer(pa, nullptr, { pda,true }, cursor, declarators);
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
	ParseDeclaratorWithInitializer(pa, typeResult, { pda,false }, cursor, declarators);
}

/***********************************************************************
ParseDeclarator (Helpers)
***********************************************************************/

void ParseMemberDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators)
{
	ParseDeclarator(pa, pda, true, cursor, declarators);
}

void ParseNonMemberDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, Ptr<Type> type, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators)
{
	ParseDeclaratorWithInitializer(pa, type, { pda,false }, cursor, declarators);
}

void ParseNonMemberDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, Ptr<CppTokenCursor>& cursor, List<Ptr<Declarator>>& declarators)
{
	ParseDeclarator(pa, pda, false, cursor, declarators);
}

Ptr<Declarator> ParseNonMemberDeclarator(const ParsingArguments& pa, const ParsingDeclaratorArguments& pda, Ptr<CppTokenCursor>& cursor)
{
	List<Ptr<Declarator>> declarators;
	ParseNonMemberDeclarator(pa, pda, cursor, declarators);
	if (declarators.Count() != 1) throw StopParsingException(cursor);
	return declarators[0];
}

Ptr<Type> ParseType(const ParsingArguments& pa, Ptr<CppTokenCursor>& cursor)
{
	return ParseNonMemberDeclarator(pa, pda_Type(), cursor)->type;
}